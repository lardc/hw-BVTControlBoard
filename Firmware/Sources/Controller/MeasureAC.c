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
typedef enum __ACProcessState
{
	ACPS_None = 0,
	ACPS_Ramp,
	ACPS_Plate,
	ACPS_Brake
} ACProcessState;

typedef struct __RingBufferElement
{
	Int16S Voltage;
	Int16S Current;
} RingBufferElement, *pRingBufferElement;

typedef _iq (*CurrentCalc)(Int32S RawValue, Boolean RMSFineCorrection);

// Variables
static RingBufferElement RingBuffer[SINE_PERIOD_PULSES];
static Int16U RingBufferPointer;
static Boolean RingBufferFull;

static Int16S MinSafePWM, PWM;
static Int16U RawZeroVoltage, RawZeroCurrent;
static _iq TransAndPWMCoeff, Ki_err, Kp, Ki;
static _iq TargetVrms, ControlVrms, PeriodCorrection, VrmsRateStep, LimitIrms;
static Int32U TimeCounter, PlateCounterTop;
static Boolean DbgMutePWM, DbgSRAM;

static volatile ACProcessState State = ACPS_None;
static CurrentCalc MAC_CurrentCalc;

// Forward functions
static Int16S MAC_CalculatePWM();
static void MAC_HandleVI(pDataSampleIQ Instant, pDataSampleIQ RMS);
static _iq MAC_SQRoot(Int32U Value);
static _iq MAC_PeriodController();
static void MAC_ControlCycle();
static void MAC_CacheVariables();

// Functions
Boolean MAC_StartProcess()
{
	MAC_CacheVariables();

	// Enable PWM generation
	ZwPWMB_SetValue12(0);
	ZwPWM_Enable(TRUE);
	
	// Enable control cycle
	CONTROL_SubcribeToCycle(MAC_ControlCycle);
	CONTROL_SwitchRTCycle(TRUE);
	
	return TRUE;
}
// ----------------------------------------

void MAC_FinishProcess()
{
	CONTROL_SwitchRTCycle(FALSE);
}
// ----------------------------------------

void inline MAC_SetPWM(Int16S pwm)
{
	ZwPWMB_SetValue12(DbgMutePWM ? 0 : pwm);
}
// ----------------------------------------

void MAC_Stop(Int16U Reason)
{
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MAC_PeriodController, "ramfuncs");
#endif
static _iq MAC_PeriodController(_iq ActualVrms)
{
	_iq err = ControlVrms - ActualVrms;
	Ki_err += _IQmpy(err, Ki);

	return Ki_err + _IQmpy(err, Kp);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MAC_HandleVI, "ramfuncs");
#endif
static void MAC_HandleVI(pDataSampleIQ Instant, pDataSampleIQ RMS)
{
	// Сохранение значения в кольцевой буфер
	RingBufferElement RingSample;
	RingSample.Voltage = (Int32S)SS_Voltage - RawZeroVoltage;
	RingSample.Current = (Int32S)SS_Current - RawZeroCurrent;

	RingBuffer[RingBufferPointer] = RingSample;
	RingBufferPointer++;

	if(RingBufferPointer >= SINE_PERIOD_PULSES)
	{
		RingBufferPointer = 0;
		RingBufferFull = TRUE;
	}

	// Сохранение пересчитанных мгновенных значений
	Instant->Voltage = MU_CalcVoltage(RingSample.Voltage, FALSE);
	Instant->Current = MAC_CurrentCalc(RingSample.Current, FALSE);

	// Расчёт действующих значений
	Int32U Vrms_sum = 0, Irms_sum = 0;
	Int16U i, cnt = RingBufferFull ? SINE_PERIOD_PULSES : RingBufferPointer;
	for(i = 0; i < cnt; i++)
	{
		Vrms_sum += (Int32S)RingBuffer[i].Voltage * RingBuffer[i].Voltage;
		Irms_sum += (Int32S)RingBuffer[i].Current * RingBuffer[i].Current;
	}

	RMS->Voltage = MU_CalcVoltage(MAC_SQRoot(Vrms_sum / cnt), TRUE);
	RMS->Current = MAC_CurrentCalc(MAC_SQRoot(Irms_sum / cnt), TRUE);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MAC_SQRoot, "ramfuncs");
#endif
static _iq MAC_SQRoot(Int32U Value)
{
	_iq2 iq2_rms = _IQ2sqrt(_IQ2mpyI32(_IQ2(1), Value));
	return _IQ2toIQ(iq2_rms);
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

	// Циклическая работа регулятора
	if(TimeCounter % SINE_PERIOD_PULSES == 0)
	{
		_iq PeriodCorrection = MAC_PeriodController(RMS.Voltage);

		if(State == ACPS_Ramp)
		{
			ControlVrms += VrmsRateStep;
			if(ControlVrms > TargetVrms)
			{
				ControlVrms = TargetVrms;
				State = ACPS_Plate;
			}
		}
	}

	// Расчёт и уставка ШИМ
	if(State == ACPS_Brake)
		PWM = (ABS(PWM) >= PWM_REDUCE_RATE) ? (PWM - SIGN(PWM) * PWM_REDUCE_RATE) : 0;
	else
		PWM = MAC_CalculatePWM();
	MAC_SetPWM(PWM);

	// Логгирование данных
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

static void MAC_CacheVariables()
{
	TargetVrms = _IQI(DataTable[REG_TARGET_VOLTAGE]);
	LimitIrms = _IQI(DataTable[REG_LIMIT_CURRENT_mA]) + _FPtoIQ2(DataTable[REG_LIMIT_CURRENT_uA], 1000);
	
	ControlVrms = _IQI(DataTable[REG_START_VOLTAGE]);

	Kp = _FPtoIQ2(DataTable[REG_KP], 100);
	Ki = _FPtoIQ2(DataTable[REG_KI], 100);
	
	VrmsRateStep = _FPtoIQ2(DataTable[REG_VOLTAGE_RATE], 10 * SINE_FREQUENCY);
	PlateCounterTop = CONTROL_FREQUENCY * DataTable[REG_VOLTAGE_PLATE_TIME];
	
	TransAndPWMCoeff = _FPtoIQ2(ZW_PWM_DUTY_BASE, DataTable[REG_PRIM_VOLTAGE] * DataTable[REG_TRANSFORMER_COFF]);
	MinSafePWM = (PWM_FREQUENCY / 1000L) * PWM_MIN_TH * ZW_PWM_DUTY_BASE / 1000000L;
	RawZeroVoltage = DataTable[REG_RAW_ZERO_SVOLTAGE];
	RawZeroCurrent = DataTable[REG_RAW_ZERO_SCURRENT];
	
	DbgSRAM = DataTable[REG_DBG_SRAM] ? TRUE : FALSE;
	DbgMutePWM = DataTable[REG_DBG_MUTE_PWM] ? TRUE : FALSE;
	
	MU_InitCoeffVoltage();
	MU_InitCoeffCurrent1();
	MU_InitCoeffCurrent2();
	MU_InitCoeffCurrent3();

	MAC_CurrentCalc = MU_CalcCurrent1;

	// Сброс переменных
	PWM = 0;
	TimeCounter = 0;
	Ki_err = 0;
	PeriodCorrection = 0;
	State = ACPS_Ramp;
}
// ----------------------------------------
