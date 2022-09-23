// ----------------------------------------
// Measuring logic AC
// ----------------------------------------

// Header
#include "MeasureAC.h"

// Includes
#include "SysConfig.h"
#include "ZwDSP.h"
#include "ZbBoard.h"
#include "SecondarySampling.h"
#include "PrimarySampling.h"
#include "DataTable.h"
#include "DeviceObjectDictionary.h"
#include "IQmathUtils.h"
#include "Controller.h"
#include "Global.h"
#include "MeasureUtils.h"
#include "DataLogger.h"

// Definitions
#define SQROOT2						_IQ(1.4142f)
#define SINE_FREQUENCY				50
#define SINE_PERIOD_PULSES			(PWM_FREQUENCY / SINE_FREQUENCY)
#define PWM_LIMIT					(ZW_PWM_DUTY_BASE * PWM_MAX_SAT / 100)

// Types
typedef enum __ProcessState
{
	PS_None = 0,
	PS_Ramp,
	PS_Plate,
	PS_Break
} ProcessState;

typedef struct __RingBufferElement
{
	Int32U Voltage;
	Int32U Current;
} RingBufferElement, *pRingBufferElement;

typedef _iq (*CurrentCalc)(Int32S RawValue, Boolean RMSFineCorrection);

// Variables
static RingBufferElement RingBuffer[SINE_PERIOD_PULSES];
static Int16U RingBufferPointer;
static Boolean RingBufferFull;

static Int16S MinSafePWM, PWM;
static Int16U RawZeroVoltage, RawZeroCurrent, FECounter, FECounterMax;
static _iq TransAndPWMCoeff, Ki_err, Kp, Ki, FEAbsolute, FERelative;
static _iq TargetVrms, ControlVrms, PeriodCorrection, VrmsRateStep, LimitIrms;
static Int32U TimeCounter, PlateCounterTop, Vsq_sum, Isq_sum;
static Boolean DbgMutePWM, DbgSRAM;

static ProcessState State;
static ProcessBreakReason BreakReason;
static CurrentCalc MAC_CurrentCalc;

// Forward functions
static Int16S MAC_CalculatePWM();
static void MAC_HandleVI(pDataSampleIQ Instant, pDataSampleIQ RMS);
static _iq MAC_SQRoot(Int32U Value);
static _iq MAC_PeriodController();
static void MAC_ControlCycle();
static Boolean MAC_InitStartState();

// Functions
Boolean MAC_StartProcess()
{
	if(!MAC_InitStartState())
		return FALSE;

	// Enable control cycle
	CONTROL_SubcribeToCycle(MAC_ControlCycle);
	CONTROL_SwitchRTCycle(TRUE);
	
	return TRUE;
}
// ----------------------------------------

void inline MAC_SetPWM(Int16S pwm)
{
	ZwPWMB_SetValue12(DbgMutePWM ? 0 : pwm);
}
// ----------------------------------------

void MAC_RequestStop(ProcessBreakReason Reason)
{
	if(State != PS_Break)
	{
		State = PS_Break;
		BreakReason = Reason;
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MAC_PeriodController, "ramfuncs");
#endif
static _iq MAC_PeriodController(_iq ActualVrms)
{
	_iq err = ControlVrms - ActualVrms;
	Ki_err += _IQmpy(err, Ki);
	PeriodCorrection = Ki_err + _IQmpy(err, Kp);

	return err;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MAC_HandleVI, "ramfuncs");
#endif
static void MAC_HandleVI(pDataSampleIQ Instant, pDataSampleIQ RMS)
{
	// Вычитание из суммы затираемого значения
	Vsq_sum -= RingBuffer[RingBufferPointer].Voltage;
	Isq_sum -= RingBuffer[RingBufferPointer].Current;

	// Сохранение нового значения в кольцевой буфер
	Int32S Vraw = (Int32S)SS_Voltage - RawZeroVoltage;
	Int32S Iraw = (Int32S)SS_Current - RawZeroCurrent;

	RingBufferElement RingSample;
	RingSample.Voltage = Vraw * Vraw;
	RingSample.Current = Iraw * Iraw;
	RingBuffer[RingBufferPointer] = RingSample;
	RingBufferPointer++;

	if(RingBufferPointer >= SINE_PERIOD_PULSES)
	{
		RingBufferPointer = 0;
		RingBufferFull = TRUE;
	}

	// Суммирование добавленного значения
	Vsq_sum += RingSample.Voltage;
	Isq_sum += RingSample.Current;

	// Сохранение пересчитанных мгновенных значений
	Instant->Voltage = MU_CalcVoltage(_IQI(Vraw), FALSE);
	Instant->Current = MAC_CurrentCalc(_IQI(Iraw), FALSE);

	// Расчёт действующих значений
	Int16U cnt = RingBufferFull ? SINE_PERIOD_PULSES : RingBufferPointer;
	RMS->Voltage = MU_CalcVoltage(MAC_SQRoot(Vsq_sum / cnt), TRUE);
	RMS->Current = MAC_CurrentCalc(MAC_SQRoot(Isq_sum / cnt), TRUE);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MAC_SQRoot, "ramfuncs");
#endif
static _iq MAC_SQRoot(Int32U Value)
{
	_iq3 iq3_rms = _IQ3sqrt(_IQ3mpyI32(_IQ3(1), Value));
	return _IQ3toIQ(iq3_rms);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MAC_ControlCycle, "ramfuncs");
#endif
static void MAC_ControlCycle()
{
	// Считывание оцифрованных значений
	DataSampleIQ Instant, RMS;
	MAC_HandleVI(&Instant, &RMS);

	// Проверка превышения значения тока
	//if(RMS.Current >= LimitIrms)
		//MAC_RequestStop(PBR_CurrentLimit);

	// Работа амплитудного регулятора
	if(TimeCounter % SINE_PERIOD_PULSES == 0)
	{
		_iq PeriodError = MAC_PeriodController(RMS.Voltage);
		MU_LogScopeError(PeriodError);

		// Проверка на ошибку следования
		if(State != PS_Break && Kp && Ki &&
				_IQdiv(_IQabs(PeriodError), ControlVrms) > FERelative && _IQabs(PeriodError) > FEAbsolute)
		{
			FECounter++;
		}
		else
			FECounter = 0;

		// Триггер ошибки следования
		if(FECounter >= FECounterMax)
			MAC_RequestStop(PBR_FollowingError);

		// Нормальное функционирование
		else
		{
			switch(State)
			{
				case PS_Ramp:
					ControlVrms += VrmsRateStep;
					if(ControlVrms > TargetVrms)
					{
						PlateCounterTop += TimeCounter;
						ControlVrms = TargetVrms;
						State = PS_Plate;
					}
					break;

				case PS_Plate:
					if(TimeCounter >= PlateCounterTop)
						State = PS_Break;
					break;
			}
		}
	}

	// Расчёт и уставка ШИМ
	if(State == PS_Break)
	{
		PWM = (ABS(PWM) >= PWM_REDUCE_RATE) ? (PWM - SIGN(PWM) * PWM_REDUCE_RATE) : 0;

		// Завершение процесса
		if(PWM == 0)
		{
			CONTROL_SwitchRTCycle(FALSE);
			CONTROL_SubcribeToCycle(NULL);

			switch(BreakReason)
			{
				case PBR_None:
				case PBR_CurrentLimit:
					DataTable[REG_RESULT_V] = _IQint(RMS.Voltage);
					DataTable[REG_RESULT_I_mA] = _IQint(RMS.Current);
					DataTable[REG_RESULT_I_uA] = _IQmpyI32int(_IQfrac(RMS.Current), 1000);
					DataTable[REG_FINISHED] = OPRESULT_OK;
					break;

				case PBR_FollowingError:
					DataTable[REG_PROBLEM] = PROBLEM_FOLLOWING_ERROR;
					DataTable[REG_FINISHED] = OPRESULT_FAIL;
					break;

				case PBR_RequestStop:
					DataTable[REG_PROBLEM] = PROBLEM_STOP;
					DataTable[REG_FINISHED] = OPRESULT_FAIL;
					break;

				case PBR_PWMSaturation:
					DataTable[REG_PROBLEM] = PROBLEM_PWM_SATURATION;
					DataTable[REG_FINISHED] = OPRESULT_FAIL;
					break;
			}

			CONTROL_RequestStop();
		}
	}
	else
	{
		PWM = MAC_CalculatePWM();
		if(ABS(PWM) == PWM_LIMIT)
			MAC_RequestStop(PBR_PWMSaturation);
	}
	MAC_SetPWM(PWM);

	// Логгирование мгновенных данных
	MU_LogScopeValues(&Instant, &RMS, PWM, DbgSRAM);
	TimeCounter++;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MAC_CalculatePWM, "ramfuncs");
#endif
static Int16S MAC_CalculatePWM()
{
	// Расчёт мгновенного значения напряжения
	// Отбрасывание целых периодов счётчика времени
	Int32U TrimmedCounter = TimeCounter - (TimeCounter % SINE_PERIOD_PULSES) * SINE_PERIOD_PULSES;
	_iq SinValue = _IQsinPU(_FPtoIQ2(TrimmedCounter, SINE_PERIOD_PULSES));
	_iq InstantVoltage = _IQmpy(_IQmpy(SQROOT2, ControlVrms + PeriodCorrection), SinValue);

	// Пересчёт в ШИМ
	Int16S pwm = _IQint(_IQmpy(InstantVoltage, TransAndPWMCoeff));

	// Обрезка верхних значений
	if(ABS(pwm) > PWM_LIMIT)
		return SIGN(pwm) * PWM_LIMIT;

	// Обрезка нижних значений
	else if(ABS(pwm) < (MinSafePWM / 2))
		return 0;
	else if(ABS(pwm) < MinSafePWM)
		return MinSafePWM * SIGN(pwm);
	else
		return pwm;
}
// ----------------------------------------

static Boolean MAC_InitStartState()
{
	TargetVrms = _IQI(DataTable[REG_TARGET_VOLTAGE]);
	LimitIrms = _IQI(DataTable[REG_LIMIT_CURRENT_mA]) + _FPtoIQ2(DataTable[REG_LIMIT_CURRENT_uA], 1000);
	
	ControlVrms = _IQI(DataTable[REG_START_VOLTAGE]);

	Kp = _FPtoIQ2(DataTable[REG_KP], 100);
	Ki = _FPtoIQ2(DataTable[REG_KI], 100);
	
	VrmsRateStep = _FPtoIQ2(DataTable[REG_VOLTAGE_RATE] * 100, SINE_FREQUENCY);
	PlateCounterTop = CONTROL_FREQUENCY * DataTable[REG_VOLTAGE_PLATE_TIME];
	
	TransAndPWMCoeff = _FPtoIQ2(ZW_PWM_DUTY_BASE, DataTable[REG_PRIM_VOLTAGE] * DataTable[REG_TRANSFORMER_COFF]);
	MinSafePWM = (PWM_FREQUENCY / 1000L) * PWM_MIN_TH * ZW_PWM_DUTY_BASE / 1000000L;
	RawZeroVoltage = DataTable[REG_RAW_ZERO_SVOLTAGE];
	RawZeroCurrent = DataTable[REG_RAW_ZERO_SCURRENT];
	
	FEAbsolute = _IQI(DataTable[REG_FE_ABSOLUTE]);
	FERelative = _FPtoIQ2(DataTable[REG_FE_RELATIVE], 100);
	FECounterMax = DataTable[REG_FE_COUNTER_MAX];

	DbgSRAM = DataTable[REG_DBG_SRAM] ? TRUE : FALSE;
	DbgMutePWM = DataTable[REG_DBG_MUTE_PWM] ? TRUE : FALSE;
	
	MU_InitCoeffVoltage();
	MU_InitCoeffCurrent1();
	MU_InitCoeffCurrent2();
	MU_InitCoeffCurrent3();

	// Сброс переменных
	PWM = 0;
	FECounter = TimeCounter = 0;
	Ki_err = PeriodCorrection = 0;

	Vsq_sum = Isq_sum = 0;
	// Очистка кольцевого буфера
	Int16U i;
	for(i = 0; i < SINE_PERIOD_PULSES; i++)
	{
		RingBuffer[i].Voltage = 0;
		RingBuffer[i].Current = 0;
	}
	RingBufferFull = FALSE;
	RingBufferPointer = 0;

	State = PS_Ramp;
	BreakReason = PBR_None;

	// Конфигурация оцифровщика
	Boolean res;
	if(LimitIrms <= I_RANGE1)
	{
		MAC_CurrentCalc = MU_CalcCurrent1;
		res = SS_SelectShunt(SwitchConfig_I1);
	}
	else if(LimitIrms <= I_RANGE2)
	{
		MAC_CurrentCalc = MU_CalcCurrent2;
		res = SS_SelectShunt(SwitchConfig_I2);
	}
	else
	{
		MAC_CurrentCalc = MU_CalcCurrent3;
		res = SS_SelectShunt(SwitchConfig_I3);
	}

	if(res)
	{
		// Первый запрос данных
		res = SS_GetData(TRUE);

		// Задержка на переключение оптопар
		if(res)
			DELAY_US(5000);
	}

	return res;
}
// ----------------------------------------
