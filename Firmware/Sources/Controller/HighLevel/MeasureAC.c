// ----------------------------------------
// Measuring logic AC
// ----------------------------------------

// Header
#include "MeasureAC.h"
//
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
#include "PowerDriver.h"
#include "FIRFilter.h"

// Definitions
//
#define PEAK_DETECTOR_SIZE			500
#define PEAK_THR_COLLECT			_IQ(0.7f)

// Types
//
typedef enum __ACProcessState
{
	ACPS_None = 0,
	ACPS_Ramp,
	ACPS_VPrePlate,
	ACPS_VPlate,
	ACPS_Brake
} ACProcessState;

// Variables
//
static Int32U TimeCounter, StartPauseTimeCounterTop;
static Int32U VRateCounter, VRateCounterTop;
static Int32U VPlateTimeCounter, VPlateTimeCounterTop, VPrePlateTimeCounter, VPrePlateTimeCounterTop;
static Int32U BrakeTimeCounter, BrakeTimeCounterTop, FrequencyDivisorCounter, FrequencyDivisorCounterTop;
static Int16U NormalizedPIdiv2Shift;
static Int16S PeakShiftTicks;
static Int16U OptoConnectionMon, OptoConnectionMonMax, CurrentMultiply;
static Int16U FollowingErrorCounter;
static Int16S MaxSafePWM, MinSafePWM, SSVoltageP2, SSCurrentP2;
static _iq SSVoltageCoff, SSCurrentCoff, SSVoltageP1, SSVoltageP0, SSCurrentP1, SSCurrentP0, TransCoffInv, PWMCoff;
static _iq LimitCurrent, LimitCurrentHaltLevel, LimitVoltage, VoltageRateStep, NormalizedFrequency;
static _iq KpVAC, KiVAC, SIVAerr;
static _iq FollowingErrorFraction, FollowingErrorAbsolute;
static _iq ResultV, ResultI;
static _iq DesiredAmplitudeV, DesiredAmplitudeVHistory, ControlledAmplitudeV, DesiredVoltageHistory;
static _iq ActualMaxPosVoltage, ActualMaxPosCurrent;
static _iq MaxPosVoltage, MaxPosCurrent, MaxPosInstantCurrent, PeakThresholdDetect;
static DataSample ActualSecondarySample;
static Boolean TripConditionDetected, UseInstantMethod, FrequencyRateSwitch, ModifySine, DUTOpened;
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
static void MEASURE_AC_ControlCycle();
static void MEASURE_AC_CCSub_CorrectionAndLog(Int16S ActualCorrection);
static Int16S MEASURE_AC_CCSub_Regulator(Boolean *PeriodTrigger);
//
static void MEASURE_AC_CacheVariables();
static Boolean MEASURE_AC_PIControllerSequence(_iq DesiredV);
static Int16S MEASURE_AC_PredictControl(_iq DesiredV);
static void MEASURE_AC_HandleVI();
static void MEASURE_AC_HandleTripCondition(Boolean UsePeakValues);
static void MEASURE_AC_HandleNonTripCondition();
static _iq MEASURE_AC_GetCurrentLimit();
static Boolean MEASURE_AC_PWMZeroDetector();

// Functions
//
Boolean MEASURE_AC_StartProcess(Int16U Type, pInt16U pDFReason, pInt16U pProblem)
{
	// Cache data
	MEASURE_AC_CacheVariables();
	// Enable RT cycle
	CONTROL_SwitchRTCycle(TRUE);
	
	// Init variables
	Problem = PROBLEM_NONE;
	Warning = WARNING_NONE;
	Fault = DF_NONE;
	DUTOpened = TripConditionDetected = FALSE;
	OptoConnectionMon = 0;
	TimeCounter = VRateCounter = 0;
	VPrePlateTimeCounter = VPlateTimeCounter = BrakeTimeCounter = 0;
	FrequencyRateSwitch = FALSE;
	FollowingErrorFraction = FollowingErrorAbsolute = 0;
	FollowingErrorCounter = 0;
	//
	AmplitudePeriodCounter = 0;
	InvertPolarity = PWM_USE_BRIDGE_RECTIF;
	SkipRegulation = TRUE;
	SIVAerr = 0;
	//
	ActualSecondarySample.IQFields.Voltage = 0;
	ActualSecondarySample.IQFields.Current = 0;
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
	//
	FIR_Reset();
	
	// Configure samplers
	SS_ConfigureSensingCircuits(LimitCurrent, LimitVoltage);
	// Start sampling
	SS_StartSampling();
	SS_Dummy(TRUE);
	
	// Enable PWM generation
	ZwPWMB_SetValue12(0);
	ZwPWM_Enable(TRUE);
	
	// Enable control cycle
	CONTROL_SubcribeToCycle(MEASURE_AC_ControlCycle);
	
	return TRUE;
}
// ----------------------------------------

void MEASURE_AC_FinishProcess()
{
	SS_StopSampling();
	SS_Dummy(TRUE);
	
	CONTROL_SwitchRTCycle(FALSE);
}
// ----------------------------------------

Int16S inline MEASURE_AC_PredictControl(_iq DesiredV)
{
	return _IQint(_IQmpy(_IQmpy(DesiredV, TransCoffInv), PWMCoff));
}
// ----------------------------------------

Int16S inline MEASURE_AC_TrimPWM(Int16S Duty)
{
	if(ABS(Duty) < (MinSafePWM / 2))
		return 0;
	else if(ABS(Duty) < MinSafePWM)
		return MinSafePWM * SIGN(Duty);
	else
		return Duty;
}
// ----------------------------------------

Int16S inline MEASURE_AC_SetPWM(Int16S Duty)
{
	if(PWM_USE_BRIDGE_RECTIF)
	{
		Int16S PWMOutput = 0;

		if(Duty >= 0 && PrevDuty < 0)
			InvertPolarity = !InvertPolarity;

		if(Duty > 0)
		{
			PWMOutput = MEASURE_AC_TrimPWM(InvertPolarity ? -Duty : Duty);
			ZwPWMB_SetValue12(DbgMutePWM ? 0 : PWMOutput);
		}
		else
			ZwPWMB_SetValue12(0);

		PrevDuty = Duty;
		return PWMOutput;
	}
	else
	{
		Int16S PWMOutput = MEASURE_AC_TrimPWM(Duty);
		ZwPWMB_SetValue12(PWMOutput);
		return PWMOutput;
	}
}
// ----------------------------------------

void MEASURE_AC_Stop(Int16U Reason)
{
	if(Reason == DF_INTERNAL || Reason == PROBLEM_OUTPUT_SHORT || Reason == DF_BRIDGE_SHORT)
	{
		ZbGPIO_SwitchSYNC(TRUE);
		TripConditionDetected = TRUE;
	}

	switch(Reason)
	{
		case DF_INTERNAL:
			MEASURE_AC_HandleTripCondition(UseInstantMethod);
			break;

		case PROBLEM_OUTPUT_SHORT:
			MEASURE_AC_HandleTripCondition(FALSE);
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
#pragma CODE_SECTION(MEASURE_AC_CalculatePWM, "ramfuncs");
#endif
static Int16S MEASURE_AC_CalculatePWM(_iq DesiredV)
{
	Int16S correction;
	correction = MEASURE_AC_PredictControl(DesiredV) * (FrequencyRateSwitch ? 1 : 0);
	
	if(ABS(correction) > MaxSafePWM)
	{
		MEASURE_AC_Stop(DF_PWM_SATURATION);
		return 0;
	}
	
	return correction;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MEASURE_AC_HandlePeakLogic, "ramfuncs");
#endif
static void MEASURE_AC_HandlePeakLogic()
{
	Int16U i;
	
	if(UseInstantMethod)
	{
		if(ModifySine)
		{
			PeakSample.Current = MaxPosInstantCurrent;
			PeakSample.Voltage = MaxPosVoltage;
		}
		else if(DUTOpened)
		{
			PeakSample.Current = MaxPosCurrent;
			PeakSample.Voltage = MaxPosVoltage;
		}
		else
		{
			// Пороговое значение перезаписи пика — половина от уставки
			_iq CurrentThr = _IQdiv(MEASURE_AC_GetCurrentLimit(), _IQ(2));

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
		MU_LogScopeIVpeak(PeakSample);
		
		// Handle overcurrent
		if((State != ACPS_Brake) && (PeakSample.Current >= MEASURE_AC_GetCurrentLimit() || DUTOpened))
			MEASURE_AC_Stop(DF_INTERNAL);
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MEASURE_AC_PIControllerSequence, "ramfuncs");
#endif
static Boolean MEASURE_AC_PIControllerSequence(_iq DesiredV)
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
			
			MEASURE_AC_HandlePeakLogic();
			
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
				SIVAerr += _IQmpy(err, KiVAC);
				
				ControlledAmplitudeV = DesiredAmplitudeV + (SIVAerr + p);

				FollowingErrorAbsolute = err;
				FollowingErrorFraction = _IQdiv(_IQabs(err), DesiredAmplitudeV);
			}
			
			return TRUE;
		}
	}
	
	return FALSE;
}
// ----------------------------------------

void inline MEASURE_AC_DoSampling()
{
	_iq tmp, tmp2;
	_iq FilteredV, FilteredI;
	
	SS_DoSampling();
	
	FIR_LoadValues(SS_Voltage, SS_Current);
	FIR_Apply(&FilteredV, &FilteredI);
	
	tmp = _IQmpy(SSVoltageCoff, FilteredV);
	tmp2 = _IQdiv(tmp, _IQ(1000.0f));
	ActualSecondarySample.IQFields.Voltage = _IQmpy(tmp2, _IQmpyI32(tmp2, SSVoltageP2)) + _IQmpy(tmp, SSVoltageP1)
			+ SSVoltageP0;
	
	tmp = _IQmpy(SSCurrentCoff, FilteredI);
	tmp2 = _IQdiv(tmp, _IQ(1000.0f));
	ActualSecondarySample.IQFields.Current = _IQmpy(tmp2, _IQmpyI32(tmp2, SSCurrentP2)) + _IQmpy(tmp, SSCurrentP1)
			+ SSCurrentP0;

	if(!DbgDualPolarity)
	{
		if(ActualSecondarySample.IQFields.Voltage < 0)
			ActualSecondarySample.IQFields.Voltage = 0;

		if(ActualSecondarySample.IQFields.Current < 0)
			ActualSecondarySample.IQFields.Current = 0;
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MEASURE_AC_HandleVI, "ramfuncs");
#endif
static void MEASURE_AC_HandleVI()
{
	// Connectivity monitoring
	if(OptoConnectionMonMax && DBG_USE_OPTO_TIMEOUT)
	{
		if(!SS_DataValid)
		{
			if(OptoConnectionMon++ >= OptoConnectionMonMax)
				MEASURE_AC_Stop(DF_OPTO_CON_ERROR);
		}
		else
		{
			SS_DataValid = FALSE;
			OptoConnectionMon = 0;
		}
	}
	
	// Assign voltage and current values
	if(TimeCounter > StartPauseTimeCounterTop)
	{
		// Handle values
		if(ActualSecondarySample.IQFields.Current > MaxPosCurrent)
			MaxPosCurrent = ActualSecondarySample.IQFields.Current;
	}
	
	// Detect maximum voltage for AC period
	if((!ModifySine && (ActualSecondarySample.IQFields.Voltage >= MaxPosVoltage)) ||
		(ModifySine && MEASURE_AC_PWMZeroDetector()))
	{
		MaxPosVoltage = ActualSecondarySample.IQFields.Voltage;
		MaxPosInstantCurrent = ActualSecondarySample.IQFields.Current;
	}
	
	// Check current conditions
	_iq CurrentLimit = MEASURE_AC_GetCurrentLimit();
	if(UseInstantMethod)
	{
		// Проверка условия отпирания прибора
		_iq RelVoltageRatio = _IQdiv(ActualSecondarySample.IQFields.Voltage, _IQabs(DesiredVoltageHistory));
		if(OUT_SHORT_REL_V_RATIO > RelVoltageRatio && ActualSecondarySample.IQFields.Voltage < OUT_SHORT_MAX_V)
		{
			// Условие быстрой остановки ШИМ при превышении лимита тока
			if(ActualSecondarySample.IQFields.Current >= ((CurrentLimit > HVD_IL_TH) ? CurrentLimit : LimitCurrentHaltLevel))
				MEASURE_AC_Stop(PROBLEM_OUTPUT_SHORT);
			else if ((ActualSecondarySample.IQFields.Current > _IQmpy(MaxPosInstantCurrent, OUT_SHORT_REL_I_RATIO)) &&
					(ActualSecondarySample.IQFields.Voltage < _IQmpy(MaxPosVoltage, OUT_SHORT_REL_V_RATIO)) &&
					(_IQabs(DesiredVoltageHistory) > OUT_SHORT_MIN_SET_V))
			{
				DUTOpened = TRUE;
			}
		}
	}
	else
	{
		if(ActualSecondarySample.IQFields.Current >= CurrentLimit)
			MEASURE_AC_Stop(DF_INTERNAL);
	}
	
	// Store data for peak detection
	if ((ActualSecondarySample.IQFields.Voltage > _IQmpy(DesiredAmplitudeV, PEAK_THR_COLLECT)) &&
		(PeakDetectorCounter < PEAK_DETECTOR_SIZE))
	{
		PeakDetectorData[PeakDetectorCounter].Current = ActualSecondarySample.IQFields.Current;
		PeakDetectorData[PeakDetectorCounter].Voltage = ActualSecondarySample.IQFields.Voltage;
		++PeakDetectorCounter;
	}
}
// ----------------------------------------

static _iq MEASURE_AC_GetCurrentLimit()
{
	if(TimeCounter < StartPauseTimeCounterTop)
		return ((LimitCurrent < MAX_CURRENT_1ST_PULSE ) ? MAX_CURRENT_1ST_PULSE : LimitCurrent);
	else
		return LimitCurrent;
}
// ----------------------------------------

static void MEASURE_AC_HandleTripCondition(Boolean UsePeakValues)
{
	if(UsePeakValues)
	{
		ResultI = PeakSample.Current;
		ResultV = PeakSample.Voltage;
	}
	else
	{
		ResultI = ActualSecondarySample.IQFields.Current;
		ResultV = ActualSecondarySample.IQFields.Voltage;
	}
}
// ----------------------------------------

static void MEASURE_AC_HandleNonTripCondition()
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
#pragma CODE_SECTION(MEASURE_AC_ControlCycle, "ramfuncs");
#endif
static void MEASURE_AC_ControlCycle()
{
	Int16S correction = 0;
	Boolean trig_flag = FALSE;
	static Int16S PrevCorrection = 0;
	
	MEASURE_AC_DoSampling();
	TimeCounter++;
	
	switch (State)
	{
		case ACPS_Ramp:
			{
				MEASURE_AC_HandleVI();
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
				
				correction = MEASURE_AC_CCSub_Regulator(NULL);
				if(State == ACPS_Brake)
					return;
				MEASURE_AC_CCSub_CorrectionAndLog(correction);
			}
			break;
			
		case ACPS_VPrePlate:
			{
				MEASURE_AC_HandleVI();
				VPrePlateTimeCounter++;
				
				correction = MEASURE_AC_CCSub_Regulator(&trig_flag);
				if(State == ACPS_Brake)
					return;
				
				if(trig_flag
						&& (PRE_PLATE_MAX_ERR >= _IQabs(FollowingErrorAbsolute)
								|| VPrePlateTimeCounter >= VPrePlateTimeCounterTop))
				{
					State = ACPS_VPlate;
					DataTable[REG_VOLTAGE_ON_PLATE] = 1;
				}
				
				MEASURE_AC_CCSub_CorrectionAndLog(correction);
			}
			break;
			
		case ACPS_VPlate:
			{
				MEASURE_AC_HandleVI();
				VPlateTimeCounter++;
				
				correction = MEASURE_AC_CCSub_Regulator(&trig_flag);
				if(State == ACPS_Brake)
					return;
				
				if(VPlateTimeCounter > VPlateTimeCounterTop && trig_flag)
					State = ACPS_Brake;
				else
					MEASURE_AC_CCSub_CorrectionAndLog(correction);
			}
			break;
			
		case ACPS_Brake:
			{
				if(Problem == PROBLEM_OUTPUT_SHORT)
					PrevCorrection = 0;

				// Reduce correction value smoothly
				correction =
						(ABS(PrevCorrection) >= PWM_REDUCE_RATE) ?
								(PrevCorrection - SIGN(PrevCorrection) * PWM_REDUCE_RATE) : 0;
				MEASURE_AC_CCSub_CorrectionAndLog((Warning == WARNING_OUTPUT_OVERLOAD) ? 0 : correction);
				
				// Increase timer only when PWM reduced to zero
				if(correction == 0)
					++BrakeTimeCounter;
				
				if(BrakeTimeCounter >= BrakeTimeCounterTop)
				{
					CONTROL_SubcribeToCycle(NULL);
					
					if(!TripConditionDetected)
						MEASURE_AC_HandleNonTripCondition();
					
					CONTROL_NotifyEndTest(ResultV, ResultI, Fault, Problem, Warning);
					ZwPWM_Enable(FALSE);
					ZbGPIO_SwitchSYNC(FALSE);
					State = ACPS_None;
				}
			}
			break;
	}
	
	PrevCorrection = correction;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MEASURE_AC_CCSub_CorrectionAndLog, "ramfuncs");
#endif
static void MEASURE_AC_CCSub_CorrectionAndLog(Int16S ActualCorrection)
{
	Int16S PWMOutput = MEASURE_AC_SetPWM(ActualCorrection);
	if(!SkipLoggingVoids || FrequencyRateSwitch)
	{
		if(!(SkipNegativeLogging && InvertPolarity))
		{
			MU_LogScope(&ActualSecondarySample, CurrentMultiply, DbgSRAM, DbgDualPolarity);
			MU_LogScopeIV(ActualSecondarySample);
			MU_LogScopeDIAG(PWMOutput);
		}
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(MEASURE_AC_CCSub_Regulator, "ramfuncs");
#endif
static Int16S MEASURE_AC_CCSub_Regulator(Boolean *PeriodTrigger)
{
	Boolean ret;
	Int16S correction;
	_iq desiredSecondaryVoltage;
	
	// Calculate desired amplitude
	_iq SineValue = _IQsinPU(_IQmpyI32(NormalizedFrequency, TimeCounter));
	if(ModifySine)
		desiredSecondaryVoltage = _IQmpy(_IQmpy(SineValue, _IQexp(_IQ(1) - _IQabs(SineValue))), ControlledAmplitudeV);
	else
		desiredSecondaryVoltage = _IQmpy(SineValue, ControlledAmplitudeV);

	// Calculate correction
	ret = MEASURE_AC_PIControllerSequence(desiredSecondaryVoltage);
	if(PeriodTrigger)
		*PeriodTrigger = ret;
	// Transform correction to PWM value
	correction = MEASURE_AC_CalculatePWM(desiredSecondaryVoltage);
	DesiredVoltageHistory = desiredSecondaryVoltage;
	
	// Following error detection
	if(ret && !DbgMutePWM && DBG_USE_FOLLOWING_ERR && (KpVAC != 0) && (KiVAC != 0))
	{
		if((FollowingErrorFraction > FE_MAX_FRACTION) && (_IQabs(FollowingErrorAbsolute) > FE_MAX_ABSOLUTE))
		{
			if(FollowingErrorCounter++ > (FE_MAX_COUNTER * (PWM_SKIP_NEG_PULSES ? 2 : 1)))
			{
				correction = 0;
				MEASURE_AC_Stop(DF_FOLLOWING_ERROR);
			}
		}
		else
			FollowingErrorCounter = 0;
	}
	
	// Log error
	if(ret)
		MU_LogScopeErr(_IQint(FollowingErrorAbsolute));
	
	return correction;
}
// ----------------------------------------

static Boolean MEASURE_AC_PWMZeroDetector()
{
	static _iq PrevSine = 0;
	Boolean result = FALSE;
	_iq CurrentSine = _IQsinPU(_IQmpyI32(NormalizedFrequency, TimeCounter + NormalizedPIdiv2Shift + PeakShiftTicks));

	if(((PrevSine < 0 && CurrentSine >= 0) || (PrevSine > 0 && CurrentSine <= 0)) && PrevDuty > 0)
		result = TRUE;

	PrevSine = CurrentSine;
	return result;
}
// ----------------------------------------

static void MEASURE_AC_CacheVariables()
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
	
	TransCoffInv = _FPtoIQ2(1, DataTable[REG_TRANSFORMER_COFF]);
	PWMCoff = _IQdiv(_IQ(ZW_PWM_DUTY_BASE), _IQI(DataTable[REG_PRIM_VOLTAGE_CTRL]));
	MaxSafePWM = DataTable[REG_SAFE_MAX_PWM];
	
	StartPauseTimeCounterTop = (CONTROL_FREQUENCY / DataTable[REG_VOLTAGE_FREQUENCY]) * 2;
	NormalizedFrequency = _IQdiv(_IQ(1.0f), _IQI(CONTROL_FREQUENCY / DataTable[REG_VOLTAGE_FREQUENCY]));
	NormalizedPIdiv2Shift = CONTROL_FREQUENCY / (4L * DataTable[REG_VOLTAGE_FREQUENCY]);
	VoltageRateStep = _IQmpy(_IQdiv(_IQI(1000 / (PWM_SKIP_NEG_PULSES ? 2 : 1)), _IQI(DataTable[REG_VOLTAGE_FREQUENCY])),
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
