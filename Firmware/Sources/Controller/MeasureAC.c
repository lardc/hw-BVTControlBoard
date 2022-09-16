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
#define PEAK_DETECTOR_SIZE			500
#define PEAK_THR_COLLECT			_IQ(0.7f)

#define SQROOT2						_IQ(1.4142f)
#define SINE_FREQUENCY				50
#define SINE_PERIOD_PULSES			(PWM_FREQUENCY / SINE_FREQUENCY)

// Types
typedef enum __ACProcessState
{
	ACPS_None = 0,
	ACPS_Ramp,
	ACPS_VPrePlate,
	ACPS_VPlate,
	ACPS_Brake
} ACProcessState;

typedef struct __MeasureCoeff
{
	_iq K;
	_iq Offset;
	_iq P2;
	_iq P1;
	_iq P0;
} MeasureCoeff;

// Variables
static DataSampleIQ RingBuffer[SINE_PERIOD_PULSES];
static Int16U RingBufferPointer;
static Boolean RingBufferFull;

static Int16U TargetVrms, ControlTargetVrms, MaxSafePWM, RawZeroVoltage, RawZeroCurrent;
static _iq TransAndPWMCoeff;

static Int32U TimeCounter, StartPauseTimeCounterTop;
static Int32U VRateCounter, VRateCounterTop;
static Int32U VPlateTimeCounter, VPlateTimeCounterTop, VPrePlateTimeCounter, VPrePlateTimeCounterTop;
static Int32U BrakeTimeCounter, BrakeTimeCounterTop, FrequencyDivisorCounter, FrequencyDivisorCounterTop;
static Int16U NormalizedPIdiv2Shift;
static Int16U OptoConnectionMon, OptoConnectionMonMax, CurrentMultiply;
static Int16U FollowingErrorCounter;
static Int16S MinSafePWM, SSVoltageP2, SSCurrentP2;
static _iq SSVoltageCoff, SSCurrentCoff, SSVoltageP1, SSVoltageP0, SSCurrentP1, SSCurrentP0;
static _iq LimitCurrent, LimitCurrentHaltLevel, LimitVoltage, VoltageRateStep, NormalizedFrequency;
static _iq KpVAC, KiVAC, SVIAerr;
static _iq FollowingErrorFraction, FollowingErrorAbsolute;
static _iq ResultV, ResultI;
static _iq DesiredAmplitudeV, DesiredAmplitudeVHistory, ControlledAmplitudeV, DesiredVoltageHistory;
static _iq ActualMaxPosVoltage, ActualMaxPosCurrent;
static _iq MaxPosVoltage, MaxPosCurrent, MaxPosInstantCurrent, PeakThresholdDetect;
static DataSample ActualSecondarySample;
static Boolean TripConditionDetected, UseInstantMethod, FrequencyRateSwitch, ModifySine;
static Boolean DbgDualPolarity, DbgSRAM, DbgMutePWM, SkipRegulation, SkipLoggingVoids, SkipNegativeLogging;
static Boolean InvertPolarity;
static Int16S PrevDuty;
static Int16U AmplitudePeriodCounter;
static Int16U Problem, Warning, Fault;
static DataSampleIQ PeakDetectorData[PEAK_DETECTOR_SIZE], PeakSample;
static Int16U PeakDetectorCounter;
//
static volatile ACProcessState State = ACPS_None;

// Forward functions
//
static Int16S MAC_CalculatePWM();

static void MAC_ControlCycle();
static void MAC_CCSub_CorrectionAndLog(Int16S ActualCorrection);
static void MAC_CacheVariables();
static Boolean MAC_PIControllerSequence(_iq DesiredV);
static void MAC_HandleVI();
static void MAC_HandleTripCondition(Boolean UsePeakValues);
static void MAC_HandleNonTripCondition();
static _iq MAC_GetCurrentLimit();

// Functions
//
Boolean MAC_StartProcess()
{
	MAC_CacheVariables();
	CONTROL_SwitchRTCycle(TRUE);
	
	// Init variables
	Problem = PROBLEM_NONE;
	Warning = WARNING_NONE;
	Fault = DF_NONE;
	TripConditionDetected = FALSE;
	OptoConnectionMon = 0;
	TimeCounter = VRateCounter = 0;
	VPrePlateTimeCounter = VPlateTimeCounter = BrakeTimeCounter = 0;
	FrequencyRateSwitch = FALSE;
	FollowingErrorFraction = FollowingErrorAbsolute = 0;
	FollowingErrorCounter = 0;
	//
	AmplitudePeriodCounter = 0;
	InvertPolarity = TRUE;
	SkipRegulation = TRUE;
	SVIAerr = 0;
	//
	ActualMaxPosVoltage = 0;
	ActualMaxPosCurrent = 0;
	MaxPosVoltage = 0;
	MaxPosCurrent = 0;
	MaxPosInstantCurrent = 0;
	DesiredVoltageHistory = -1;
	PrevDuty = -1;
	PeakDetectorCounter = 0;
	//
	ResultV = ResultI = 0;
	State = ACPS_Ramp;
	
	// Configure samplers
	//SS_ConfigureSensingCircuits(LimitCurrent, LimitVoltage);
	// Start sampling
	//SS_StartSampling();
	//SS_Dummy(TRUE);
	
	// Enable PWM generation
	ZwPWMB_SetValue12(0);
	ZwPWM_Enable(TRUE);
	
	// Enable control cycle
	CONTROL_SubcribeToCycle(MAC_ControlCycle);
	
	return TRUE;
}
// ----------------------------------------

void MAC_FinishProcess()
{
	//SS_StopSampling();
	//SS_Dummy(TRUE);
	
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
	TripConditionDetected = TRUE;

	switch (Reason)
	{
		case DF_INTERNAL:
			MAC_HandleTripCondition(UseInstantMethod);
			break;

		case PROBLEM_OUTPUT_SHORT:
			MAC_HandleTripCondition(FALSE);
		case DF_NONE:
			Problem = Reason;
			break;

		default:
			Fault = Reason;
			break;
	}
	
	State = ACPS_Brake;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MAC_HandlePeakLogic, "ramfuncs");
#endif
static void MAC_HandlePeakLogic()
{
	Int16U i;
	
	if (UseInstantMethod)
	{
		if (ModifySine)
		{
			PeakSample.Current = MaxPosInstantCurrent;
			PeakSample.Voltage = MaxPosVoltage;
		}
		else
		{
			// Пороговое значение перезаписи пика — половина от уставки
			_iq CurrentThr = _IQdiv(MAC_GetCurrentLimit(), _IQ(2));

			if (PeakDetectorCounter)
			{
				PeakSample.Current = MaxPosInstantCurrent;
				PeakSample.Voltage = MaxPosVoltage;
			
				// Handle peak data
				for (i = 0; i < PeakDetectorCounter; ++i)
				{
					if ((PeakDetectorData[i].Voltage > _IQmpy(MaxPosVoltage, PeakThresholdDetect)) &&
						(PeakDetectorData[i].Current > PeakSample.Current) &&
						(PeakDetectorData[i].Current > CurrentThr))
					{
						PeakSample = PeakDetectorData[i];
					}
				}
			}
			else
			{
				PeakSample.Current = 0;
				PeakSample.Voltage = 0;
			}
		}
		
		// Handle overcurrent
		if((State != ACPS_Brake) && (PeakSample.Current >= MAC_GetCurrentLimit()))
			MAC_Stop(DF_INTERNAL);
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MAC_PIControllerSequence, "ramfuncs");
#endif
static Boolean MAC_PIControllerSequence(_iq DesiredV)
{
	// Every even zero
	if((DesiredV >= 0) && (DesiredVoltageHistory < 0))
	{
		FrequencyRateSwitch = (FrequencyDivisorCounter-- == FrequencyDivisorCounterTop) ? TRUE : FALSE;
		if(FrequencyDivisorCounter == 0)
			FrequencyDivisorCounter = FrequencyDivisorCounterTop;
		
		if(FrequencyRateSwitch)
			++AmplitudePeriodCounter;

		if((PWM_SKIP_NEG_PULSES && AmplitudePeriodCounter > 1) || (!PWM_SKIP_NEG_PULSES && AmplitudePeriodCounter > 0))
		{
			AmplitudePeriodCounter = 0;
			_iq err = 0, p;
			
			MAC_HandlePeakLogic();
			
			ActualMaxPosVoltage = UseInstantMethod ? PeakSample.Voltage : MaxPosVoltage;
			ActualMaxPosCurrent = MaxPosCurrent;
			//
			MaxPosVoltage = 0;
			MaxPosCurrent = 0;
			MaxPosInstantCurrent = 0;
			//
			PeakDetectorCounter = 0;
			
			if(SkipRegulation)
			{
				SkipRegulation = FALSE;
				FollowingErrorAbsolute = FollowingErrorFraction = 0;
			}
			else
			{
				err = DesiredAmplitudeVHistory - ActualMaxPosVoltage;
				DesiredAmplitudeVHistory = DesiredAmplitudeV;
				p = _IQmpy(err, KpVAC);
				SVIAerr += _IQmpy(err, KiVAC);
				
				ControlledAmplitudeV = DesiredAmplitudeV + (SVIAerr + p);

				FollowingErrorAbsolute = err;
				FollowingErrorFraction = _IQdiv(_IQabs(err), DesiredAmplitudeV);
			}
			
			return TRUE;
		}
	}
	
	return FALSE;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MAC_HandleVI, "ramfuncs");
#endif
static void MAC_HandleVI()
{
	/*
	// Сохранение значения в кольцевой буфер
	RingBufferV[RingBufferPointer] = (Int32S)SS_Voltage - RawZeroVoltage;
	RingBufferI[RingBufferPointer] = (Int32S)SS_Current - RawZeroCurrent;
	RingBufferPointer++;

	if(RingBufferPointer >= SINE_PERIOD_PULSES)
	{
		RingBufferPointer = 0;
		RingBufferFull = TRUE;
	}

	// Расчёт действующих значений
	Int32S Vrms = 0, Irms = 0;
	Int16U i, cnt = RingBufferFull ? SINE_PERIOD_PULSES : RingBufferPointer;
	for(i = 0; i < cnt; i++)
	{
		Vrms += RingBufferV[i] * RingBufferV[i];
		Irms += RingBufferI[i] * RingBufferI[i];
	}
	*/
}
// ----------------------------------------

static _iq MAC_GetCurrentLimit()
{
	if(TimeCounter < StartPauseTimeCounterTop)
		return ((LimitCurrent < MAX_CURRENT_1ST_PULSE ) ? MAX_CURRENT_1ST_PULSE : LimitCurrent);
	else
		return LimitCurrent;
}
// ----------------------------------------

static void MAC_HandleTripCondition(Boolean UsePeakValues)
{
	if(UsePeakValues)
	{
		ResultI = PeakSample.Current;
		ResultV = PeakSample.Voltage;
	}
	else
	{
		//ResultI = ActualSecondarySample.IQFields.Current;
		//ResultV = ActualSecondarySample.IQFields.Voltage;
	}
}
// ----------------------------------------

static void MAC_HandleNonTripCondition()
{
	if(UseInstantMethod)
	{
		ResultI = PeakSample.Current;
		ResultV = PeakSample.Voltage;
	}
	else
	{
		ResultI = ActualMaxPosCurrent;
		ResultV = ActualMaxPosVoltage;
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MAC_ControlCycle, "ramfuncs");
#endif
static void MAC_ControlCycle()
{
	Int16S correction = 0;
	Boolean trig_flag = FALSE;
	static Int16S PrevCorrection = 0;
	
	TimeCounter++;
	
	switch (State)
	{
		case ACPS_Ramp:
			{
				MAC_HandleVI();
				VRateCounter++;
				
				if(VRateCounter >= VRateCounterTop)
				{
					VRateCounter = 0;
					DesiredAmplitudeV += VoltageRateStep;
					
					if(DesiredAmplitudeV >= LimitVoltage)
					{
						DesiredAmplitudeV = LimitVoltage;
						State = ACPS_VPrePlate;
					}
				}
				
				correction = MAC_CalculatePWM();
				if(State == ACPS_Brake)
					return;
				MAC_CCSub_CorrectionAndLog(correction);
			}
			break;
			
		case ACPS_VPrePlate:
			{
				MAC_HandleVI();
				VPrePlateTimeCounter++;
				
				correction = MAC_CalculatePWM();
				if(State == ACPS_Brake)
					return;
				
				if(trig_flag
						&& (PRE_PLATE_MAX_ERR >= _IQabs(FollowingErrorAbsolute)
								|| VPrePlateTimeCounter >= VPrePlateTimeCounterTop))
				{
					State = ACPS_VPlate;
					DataTable[REG_VOLTAGE_ON_PLATE] = 1;
				}
				
				MAC_CCSub_CorrectionAndLog(correction);
			}
			break;
			
		case ACPS_VPlate:
			{
				MAC_HandleVI();
				VPlateTimeCounter++;
				
				correction = MAC_CalculatePWM();
				if(State == ACPS_Brake)
					return;
				
				if(VPlateTimeCounter > VPlateTimeCounterTop && trig_flag)
					State = ACPS_Brake;
				else
					MAC_CCSub_CorrectionAndLog(correction);
			}
			break;
			
		case ACPS_Brake:
			{
				// Reduce correction value smoothly
				correction =
						(ABS(PrevCorrection) >= PWM_REDUCE_RATE) ?
								(PrevCorrection - SIGN(PrevCorrection) * PWM_REDUCE_RATE) : 0;
				MAC_CCSub_CorrectionAndLog((Warning == WARNING_OUTPUT_OVERLOAD) ? 0 : correction);
				
				// Increase timer only when PWM reduced to zero
				if(correction == 0)
					++BrakeTimeCounter;
				
				if(BrakeTimeCounter >= BrakeTimeCounterTop)
				{
					CONTROL_SubcribeToCycle(NULL);
					
					if(!TripConditionDetected)
						MAC_HandleNonTripCondition();
					
					CONTROL_NotifyEndTest(ResultV, ResultI, Fault, Problem, Warning);
					ZwPWM_Enable(FALSE);
					State = ACPS_None;
				}
			}
			break;
	}
	
	PrevCorrection = correction;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MAC_CCSub_CorrectionAndLog, "ramfuncs");
#endif
static void MAC_CCSub_CorrectionAndLog(Int16S ActualCorrection)
{
	if(!SkipLoggingVoids || FrequencyRateSwitch)
	{
		if(!(SkipNegativeLogging && InvertPolarity))
		{
			//MU_LogScopeVI(ActualSecondarySample, );
			MU_LogScopePWM(ActualCorrection);
		}
	}
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
	_iq InstantVoltage = _IQmpy(_IQmpyI32(SQROOT2, ControlTargetVrms), SinValue);

	// Пересчёт в ШИМ
	Int16S pwm = _IQint(_IQmpy(InstantVoltage, TransAndPWMCoeff));

	// Обрезка нижних значений
	if(ABS(pwm) < (MinSafePWM / 2))
		return 0;
	else if(ABS(pwm) < MinSafePWM)
		return MinSafePWM * SIGN(pwm);
	else
		return pwm;
}
// ----------------------------------------

static void MAC_CacheVariables()
{
	// Current in mA
	LimitCurrent = _FPtoIQ2(DataTable[REG_LIMIT_CURRENT], 10);
	LimitVoltage = _IQI(DataTable[REG_LIMIT_VOLTAGE]);
	FrequencyDivisorCounter = FrequencyDivisorCounterTop = DataTable[REG_FREQUENCY_DIVISOR];
	
	KpVAC = _FPtoIQ2(DataTable[REG_KP_VAC_N], DataTable[REG_KP_VAC_D]);
	KiVAC = _FPtoIQ2(DataTable[REG_KI_VAC_N], DataTable[REG_KI_VAC_D]);
	
	VRateCounterTop = CONTROL_FREQUENCY / DataTable[REG_VOLTAGE_FREQUENCY];
	VPrePlateTimeCounterTop = (CONTROL_FREQUENCY / 1000) * PRE_PLATE_MAX_TIME * (1 + FrequencyDivisorCounter / 5);
	VPlateTimeCounterTop = (CONTROL_FREQUENCY * DataTable[REG_VOLTAGE_PLATE_TIME]) / 1000;
	BrakeTimeCounterTop = (CONTROL_FREQUENCY * DataTable[REG_BRAKE_TIME]) / 1000;
	


	TransAndPWMCoeff = _FPtoIQ2(ZW_PWM_DUTY_BASE, DataTable[REG_PRIM_VOLTAGE] * DataTable[REG_TRANSFORMER_COFF]);
	MaxSafePWM = DataTable[REG_SAFE_MAX_PWM];
	RawZeroVoltage = DataTable[REG_RAW_ZERO_SVOLTAGE];
	RawZeroCurrent = DataTable[REG_RAW_ZERO_SCURRENT];
	

	StartPauseTimeCounterTop = (CONTROL_FREQUENCY / DataTable[REG_VOLTAGE_FREQUENCY]) * 2;
	NormalizedFrequency = _IQdiv(_IQ(1.0f), _IQI(CONTROL_FREQUENCY / DataTable[REG_VOLTAGE_FREQUENCY]));
	NormalizedPIdiv2Shift = CONTROL_FREQUENCY / (4L * DataTable[REG_VOLTAGE_FREQUENCY]);
	VoltageRateStep = _IQmpy(_IQdiv(_IQI((PWM_SKIP_NEG_PULSES ? 2 : 1) * 1000.0f), _IQI(DataTable[REG_VOLTAGE_FREQUENCY])),
			_IQmpyI32(_IQ(0.1f), DataTable[REG_VOLTAGE_AC_RATE]));
	MinSafePWM = (PWM_FREQUENCY / 1000L) * PWM_TH * ZW_PWM_DUTY_BASE / 1000000L;
	
	UseInstantMethod = DataTable[REG_USE_INST_METHOD] ? TRUE : FALSE;
	PeakThresholdDetect = _FPtoIQ2(DataTable[REG_PEAK_SEARCH_ZONE], 100);
	
	DbgSRAM = DataTable[REG_DBG_SRAM] ? TRUE : FALSE;
	DbgMutePWM = DataTable[REG_DBG_MUTE_PWM] ? TRUE : FALSE;
	DbgDualPolarity = DataTable[REG_DBG_DUAL_POLARITY] ? TRUE : FALSE;
	
	SkipLoggingVoids = DataTable[REG_SKIP_LOGGING_VOIDS] ? TRUE : FALSE;
	SkipNegativeLogging = DataTable[REG_SKIP_NEG_LOGGING] ? TRUE : FALSE;
	
	// Optical connection monitor
	OptoConnectionMonMax = DataTable[REG_OPTO_CONNECTION_MON];
	
	// Select start voltage basing on measurement mode
	ControlledAmplitudeV = DesiredAmplitudeV = DesiredAmplitudeVHistory = _IQI(DataTable[REG_START_VOLTAGE_AC]);
	
	ModifySine = FALSE;
	CurrentMultiply = 10;
	if(LimitCurrent <= HVD_IL_TH)
	{
		LimitCurrentHaltLevel = HVD_IL_TH;
		
		SSCurrentCoff = _FPtoIQ2(DataTable[REG_SCURRENT1_COFF_N], DataTable[REG_SCURRENT1_COFF_D]);
		SSCurrentCoff = _IQdiv(SSCurrentCoff, _IQ(100.0f));
		
		SSCurrentP2 = (Int16S)DataTable[REG_SCURRENT1_FINE_P2];
		SSCurrentP1 = _FPtoIQ2(DataTable[REG_SCURRENT1_FINE_P1], 1000);
		SSCurrentP0 = _FPtoIQ2((Int16S)DataTable[REG_SCURRENT1_FINE_P0], 1000);
	}
	else
	{
		LimitCurrentHaltLevel = HVD_IH_TH;
		
		SSCurrentCoff = _FPtoIQ2(DataTable[REG_SCURRENT2_COFF_N], DataTable[REG_SCURRENT2_COFF_D]);
		
		SSCurrentP2 = (Int16S)DataTable[REG_SCURRENT2_FINE_P2];
		SSCurrentP1 = _FPtoIQ2(DataTable[REG_SCURRENT2_FINE_P1], 1000);
		SSCurrentP0 = _FPtoIQ2((Int16S)DataTable[REG_SCURRENT2_FINE_P0], 1000);
	}
	
	if(LimitVoltage < HVD_VL_TH)
	{
		SSVoltageCoff = _FPtoIQ2(DataTable[REG_SVOLTAGE1_COFF_N], DataTable[REG_SVOLTAGE1_COFF_D]);
		
		SSVoltageP2 = (Int16S)DataTable[REG_SVOLTAGE1_FINE_P2];
		SSVoltageP1 = _FPtoIQ2(DataTable[REG_SVOLTAGE1_FINE_P1], 1000);
		SSVoltageP0 = _FPtoIQ2((Int16S)DataTable[REG_SVOLTAGE1_FINE_P0], 10);
	}
	else
	{
		SSVoltageCoff = _FPtoIQ2(DataTable[REG_SVOLTAGE2_COFF_N], DataTable[REG_SVOLTAGE2_COFF_D]);
		
		SSVoltageP2 = (Int16S)DataTable[REG_SVOLTAGE2_FINE_P2];
		SSVoltageP1 = _FPtoIQ2(DataTable[REG_SVOLTAGE2_FINE_P1], 1000);
		SSVoltageP0 = _FPtoIQ2((Int16S)DataTable[REG_SVOLTAGE2_FINE_P0], 10);
	}
}
// ----------------------------------------
