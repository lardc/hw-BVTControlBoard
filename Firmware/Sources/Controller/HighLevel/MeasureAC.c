// -----------------------------------------
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
static Int16U OptoConnectionMon, OptoConnectionMonMax, CurrentMultiply;
static Int16U FollowingErrorCounter, NormalizedPIdiv2Shift;
static Int16S MaxSafePWM, MinSafePWM, SSVoltageP2, SSCurrentP2;
static _iq SSVoltageCoff, SSCurrentCoff, SSVoltageP1, SSVoltageP0, SSCurrentP1, SSCurrentP0, TransCoffInv, PWMCoff;
static _iq LimitCurrent, LimitCurrentHaltLevel, LimitVoltage, VoltageRateStep, NormalizedFrequency;
static _iq KpVAC, KiVAC, SIVAerr;
static _iq FollowingErrorFraction, FollowingErrorAbsolute, FollowingErrorCurrentDelta;
static _iq ResultV, ResultI;
static _iq DesiredAmplitudeV, DesiredAmplitudeVHistory, ControlledAmplitudeV, DesiredVoltageHistory, SineValue;
static _iq ActualMaxPosVoltage, ActualMaxPosCurrent;
static _iq MaxPosVoltage, MaxPosCurrent, MaxPosInstantCurrent, PeakThresholdDetect;
static DataSample ActualSecondarySample;
static Boolean TripCurrentDetected, UseInstantMethod, FrequencyRateSwitch;
static Boolean DbgDualPolarity, DbgSRAM, DbgMutePWM, SkipRegulation, SkipLoggingVoids;
static Int16U MeasurementType, Problem, Warning, Fault;
#pragma DATA_SECTION(PeakDetectorData, "data_mem");
static DataSampleIQ PeakDetectorData[PEAK_DETECTOR_SIZE], PeakSample;
static DataSampleIQ PeakSampleByCurrent;
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
static void MEASURE_AC_HandleTripCondition();
static void MEASURE_AC_HandleNonTripCondition();
static _iq MEASURE_AC_GetCurrentLimit();


// Functions
//
Boolean MEASURE_AC_StartProcess(Int16U Type, pInt16U pDFReason, pInt16U pProblem)
{
	MeasurementType = Type;

	// Cache data
	MEASURE_AC_CacheVariables();
	// Enable RT cycle
	CONTROL_SwitchRTCycle(TRUE);

	// Init variables
	Problem = PROBLEM_NONE;
	Warning = WARNING_NONE;
	Fault = DF_NONE;
	TripCurrentDetected = FALSE;
	OptoConnectionMon = 0;
	TimeCounter = VRateCounter = 0;
	VPrePlateTimeCounter = VPlateTimeCounter = BrakeTimeCounter = 0;
	FrequencyRateSwitch = FALSE;
	FollowingErrorFraction = FollowingErrorAbsolute = FollowingErrorCurrentDelta = 0;
	FollowingErrorCounter = 0;
	//
	SkipRegulation = TRUE;
	SIVAerr = 0;
	SineValue = 0;
	//
	ActualSecondarySample.IQFields.VoltageRaw = 0;
	ActualSecondarySample.IQFields.Voltage = 0;
	ActualSecondarySample.IQFields.Current = 0;
	ActualMaxPosVoltage = 0;
	ActualMaxPosCurrent = 0;
	MaxPosVoltage = 0;
	MaxPosCurrent = 0;
	MaxPosInstantCurrent = 0;
	DesiredVoltageHistory = 0;
	PeakDetectorCounter = 0;
	PeakSampleByCurrent.Voltage = 0;
	PeakSampleByCurrent.Current = 0;
	//
	ResultV = ResultI = 0;
	State = ACPS_Ramp;
	//
	FIR_Reset();

	// Make proper commutation
	SS_Commutate((MeasurementType == MEASUREMENT_TYPE_AC) ? SwitchConfig_AC : SwitchConfig_BV);
	// Configure samplers
	SS_ConfigureSensingCircuits(LimitCurrent, LimitVoltage, FALSE);
	// Start sampling
	SS_StartSampling();
	SS_Dummy(TRUE);
	DELAY_US(HV_SWITCH_DELAY);

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

void inline MEASURE_AC_SwitchToBrake()
{
	State = ACPS_Brake;
}
// ----------------------------------------

Int16S inline MEASURE_AC_PredictControl(_iq DesiredV)
{
	return _IQint(_IQmpy(_IQmpy(DesiredV, TransCoffInv), PWMCoff));
}
// ----------------------------------------

Int16S inline MEASURE_AC_TrimPWM(Int16S Duty)
{
	if((ABS(Duty) < (MinSafePWM / 2)) || DbgMutePWM)
		return 0;
	else if(ABS(Duty) < MinSafePWM)
		return MinSafePWM * SIGN(Duty);
	else
		return Duty;
}
// ----------------------------------------

void inline MEASURE_AC_SetPWM(Int16S Duty)
{
	ZwPWMB_SetValue12(MEASURE_AC_TrimPWM(Duty));
}
// ----------------------------------------

void MEASURE_AC_Stop(Int16U Reason)
{
	if(Reason == DF_INTERNAL)
	{
		TripCurrentDetected = TRUE;
		MEASURE_AC_HandleTripCondition();
	}
	else if(Reason == DF_NONE || Reason == PROBLEM_OUTPUT_SHORT)
		Problem = Reason;
	else
		Fault = Reason;

	MEASURE_AC_SwitchToBrake();
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MEASURE_AC_CalculatePWM, "ramfuncs");
#endif
static Int16S MEASURE_AC_CalculatePWM(_iq DesiredV)
{
	Int16S correction;
	correction = MEASURE_AC_PredictControl(DesiredV) * (FrequencyRateSwitch ? 1 : 0);

	if(correction < -MaxSafePWM)
		correction = -MaxSafePWM;
	else if(correction > MaxSafePWM)
		correction = MaxSafePWM;

	return correction;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MEASURE_AC_HandlePeakLogic, "ramfuncs");
#endif
static void MEASURE_AC_HandlePeakLogic()
{
	Int16U i;

	if (UseInstantMethod)
	{
		if (PeakDetectorCounter)
		{
			PeakSample.Current = MaxPosInstantCurrent;
			PeakSample.Voltage = MaxPosVoltage;

			// Handle peak data
			for (i = 0; i < PeakDetectorCounter; ++i)
			{
				if ((PeakDetectorData[i].Voltage > _IQmpy(MaxPosVoltage, PeakThresholdDetect)) &&
					(PeakDetectorData[i].Current > PeakSample.Current))
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

		if(PeakSample.Current < PeakSampleByCurrent.Current)
			PeakSample.Current = PeakSampleByCurrent.Current;

		MU_LogScopeIVpeak(PeakSample);

		// Handle overcurrent
		if ((State != ACPS_Brake) && (PeakSample.Current >= MEASURE_AC_GetCurrentLimit()))
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
		if (FrequencyDivisorCounter == 0) FrequencyDivisorCounter = FrequencyDivisorCounterTop;

		if (FrequencyRateSwitch)
		{
			_iq err = 0, p;

			MEASURE_AC_HandlePeakLogic();

			// Following error parameter: current
			FollowingErrorCurrentDelta = MaxPosCurrent - ActualMaxPosCurrent;

			ActualMaxPosVoltage = UseInstantMethod ? PeakSample.Voltage : MaxPosVoltage;
			ActualMaxPosCurrent = MaxPosCurrent;
			//
			MaxPosVoltage = 0;
			MaxPosCurrent = 0;
			MaxPosInstantCurrent = 0;
			//
			PeakDetectorCounter = 0;
			PeakSampleByCurrent.Voltage = 0;
			PeakSampleByCurrent.Current = 0;

			if (!SkipRegulation)
			{
				err = DesiredAmplitudeVHistory - ActualMaxPosVoltage;
				DesiredAmplitudeVHistory = DesiredAmplitudeV;
				p = _IQmpy(err, KpVAC);
				SIVAerr += _IQmpy(err, KiVAC);

				ControlledAmplitudeV = DesiredAmplitudeV + (SIVAerr + p);
			}
			else
				SkipRegulation = FALSE;

			// Following error parameter: voltage
			FollowingErrorFraction = _IQdiv(err, DesiredAmplitudeV);
			FollowingErrorAbsolute = err;

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
	ActualSecondarySample.IQFields.Voltage = _IQmpy(tmp2, _IQmpyI32(tmp2, SSVoltageP2)) + _IQmpy(tmp, SSVoltageP1) + SSVoltageP0;

	tmp = _IQmpy(SSCurrentCoff, FilteredI);
	tmp2 = _IQdiv(tmp, _IQ(1000.0f));
	ActualSecondarySample.IQFields.Current = _IQmpy(tmp2, _IQmpyI32(tmp2, SSCurrentP2)) + _IQmpy(tmp, SSCurrentP1) + SSCurrentP0;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MEASURE_AC_HandleVI, "ramfuncs");
#endif
static void MEASURE_AC_HandleVI()
{
	_iq absVoltage, absCurrent;

	absVoltage = ABS(ActualSecondarySample.IQFields.Voltage);
	absCurrent = ABS(ActualSecondarySample.IQFields.Current);

	// Connectivity monitoring
	if(OptoConnectionMonMax)
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
		if(absCurrent > MaxPosCurrent)
			MaxPosCurrent = absCurrent;
	}

	// Detect maximum voltage for AC period
	if(absVoltage > MaxPosVoltage)
	{
		MaxPosVoltage = absVoltage;
		MaxPosInstantCurrent = absCurrent;
	}

	// Check current conditions
	if (UseInstantMethod)
	{
		if (absCurrent >= LimitCurrentHaltLevel)
			MEASURE_AC_Stop(PROBLEM_OUTPUT_SHORT);
	}
	else
	{
		if (absCurrent >= MEASURE_AC_GetCurrentLimit())
			MEASURE_AC_Stop(DF_INTERNAL);
	}

	// Store data for peak detection
	if ((absVoltage > _IQmpy(DesiredAmplitudeV, PEAK_THR_COLLECT)) &&
		(PeakDetectorCounter < PEAK_DETECTOR_SIZE))
	{
		PeakDetectorData[PeakDetectorCounter].Current = absCurrent;
		PeakDetectorData[PeakDetectorCounter].Voltage = absVoltage;
		++PeakDetectorCounter;

		_iq SineValueShifted = _IQsinPU(_IQmpyI32(NormalizedFrequency, (Int32S)TimeCounter - NormalizedPIdiv2Shift));
		if(_IQmpy(SineValue, SineValueShifted) > 0 && PeakSampleByCurrent.Current < absCurrent)
		{
			PeakSampleByCurrent.Current = absCurrent;
			PeakSampleByCurrent.Voltage = absVoltage;
		}
	}
}
// ----------------------------------------

static _iq MEASURE_AC_GetCurrentLimit()
{
	if (TimeCounter < StartPauseTimeCounterTop)
		return ((LimitCurrent < MAX_CURRENT_1ST_PULSE) ? MAX_CURRENT_1ST_PULSE : LimitCurrent);
	else
		return LimitCurrent;
}
// ----------------------------------------

static void MEASURE_AC_HandleTripCondition()
{
	if (UseInstantMethod)
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
	if (UseInstantMethod)
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
	static Int16S PrevCorrection = 0;

	MEASURE_AC_DoSampling();
	TimeCounter++;

	switch(State)
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
				MEASURE_AC_CCSub_CorrectionAndLog(correction);
			}
			break;
		case ACPS_VPrePlate:
			{
				Boolean trig_flag;

				MEASURE_AC_HandleVI();
				VPrePlateTimeCounter++;

				correction = MEASURE_AC_CCSub_Regulator(&trig_flag);

				if (trig_flag && (PRE_PLATE_MAX_ERR >= ABS(FollowingErrorAbsolute) || VPrePlateTimeCounter >= VPrePlateTimeCounterTop))
				{
					State = ACPS_VPlate;
				}

				MEASURE_AC_CCSub_CorrectionAndLog(correction);
			}
			break;
		case ACPS_VPlate:
			{
				MEASURE_AC_HandleVI();
				VPlateTimeCounter++;

				if(VPlateTimeCounter < VPlateTimeCounterTop)
					correction = MEASURE_AC_CCSub_Regulator(NULL);
				else
					MEASURE_AC_SwitchToBrake();

				MEASURE_AC_CCSub_CorrectionAndLog(correction);
			}
			break;
		case ACPS_Brake:
			{
				// Reduce correction value smoothly
				correction = (ABS(PrevCorrection) >= PWM_REDUCE_RATE) ? (PrevCorrection - SIGN(PrevCorrection) * PWM_REDUCE_RATE) : 0;
				MEASURE_AC_CCSub_CorrectionAndLog(correction);

				// Increase timer only when PWM reduced to zero
				if (correction == 0) ++BrakeTimeCounter;

				if (BrakeTimeCounter >= BrakeTimeCounterTop)
				{
					CONTROL_SubcribeToCycle(NULL);

					if (TripCurrentDetected)
					{
						if (!UseInstantMethod)
							MEASURE_AC_HandlePeakLogic();
					}
					else
						MEASURE_AC_HandleNonTripCondition();

					CONTROL_NotifyEndTest(ResultV, ResultI, _IQmpyI32(ResultI, 1000), Fault, Problem, Warning);
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
	#pragma CODE_SECTION(MEASURE_AC_CCSub_CorrectionAndLog, "ramfuncs");
#endif
static void MEASURE_AC_CCSub_CorrectionAndLog(Int16S ActualCorrection)
{
	MEASURE_AC_SetPWM(ActualCorrection);
	if (!SkipLoggingVoids || FrequencyRateSwitch)
	{
		MU_LogScope(&ActualSecondarySample, CurrentMultiply, DbgSRAM, DbgDualPolarity);
		MU_LogScopeIV(ActualSecondarySample);
		MU_LogScopeDIAG(MEASURE_AC_TrimPWM(ActualCorrection));
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
	SineValue = _IQsinPU(_IQmpyI32(NormalizedFrequency, TimeCounter));
	desiredSecondaryVoltage = _IQmpy(SineValue, ControlledAmplitudeV);
	// Calculate correction
	ret = MEASURE_AC_PIControllerSequence(desiredSecondaryVoltage);
	if (PeriodTrigger) *PeriodTrigger = ret;
	// Transform correction to PWM value
	correction = MEASURE_AC_CalculatePWM(desiredSecondaryVoltage);
	DesiredVoltageHistory = desiredSecondaryVoltage;

	// Following error detection
	if (ret && !DbgMutePWM && DBG_USE_FOLLOWING_ERR)
	{
		if (FollowingErrorFraction > FE_MAX_FRACTION && FollowingErrorCurrentDelta < FE_MIN_I_DELTA)
		{
			if (FollowingErrorCounter++ > FE_MAX_COUNTER)
			{
				correction = 0;
				MEASURE_AC_Stop(DF_FOLLOWING_ERROR);
			}
		}
		else
			FollowingErrorCounter = 0;
	}

	// Log error
	if (ret) MU_LogScopeErr(_IQint(FollowingErrorAbsolute));

	return correction;
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
	// Counter top in integer number of half-sine periods
	VPlateTimeCounterTop = ((Int32U)DataTable[REG_VOLTAGE_PLATE_TIME] * DataTable[REG_VOLTAGE_FREQUENCY] * 2 / 1000) *\
						   (CONTROL_FREQUENCY / (DataTable[REG_VOLTAGE_FREQUENCY] * 2));
	BrakeTimeCounterTop = (CONTROL_FREQUENCY * DataTable[REG_BRAKE_TIME]) / 1000;
	TransCoffInv = _FPtoIQ2(1, DataTable[REG_TRANSFORMER_COFF]);
	PWMCoff = _IQdiv(_IQ(ZW_PWM_DUTY_BASE), _IQI(DataTable[REG_PRIM_VOLTAGE_CTRL]));
	MaxSafePWM = DataTable[REG_SAFE_MAX_PWM];
	//
	StartPauseTimeCounterTop = (CONTROL_FREQUENCY / DataTable[REG_VOLTAGE_FREQUENCY]) * 2;
	NormalizedFrequency = _IQdiv(_IQ(1.0f), _IQI(CONTROL_FREQUENCY / DataTable[REG_VOLTAGE_FREQUENCY]));
	NormalizedPIdiv2Shift = CONTROL_FREQUENCY / (4L * DataTable[REG_VOLTAGE_FREQUENCY]);
	VoltageRateStep = _IQmpy(_IQdiv(_IQ(1000.0f), _IQI(DataTable[REG_VOLTAGE_FREQUENCY])),
							 _IQmpyI32(_IQ(0.1f), DataTable[REG_VOLTAGE_AC_RATE]));
	MinSafePWM = (PWM_FREQUENCY / 1000L) * PWM_TH * ZW_PWM_DUTY_BASE / 1000000L;

	UseInstantMethod = (DataTable[REG_USE_INST_METHOD] && (MeasurementType != MEASUREMENT_TYPE_AC)) ? TRUE : FALSE;
	PeakThresholdDetect = _FPtoIQ2(DataTable[REG_PEAK_SEARCH_ZONE], 100);

	DbgSRAM = DataTable[REG_DBG_SRAM] ? TRUE : FALSE;
	DbgMutePWM = DataTable[REG_DBG_MUTE_PWM] ? TRUE : FALSE;
	DbgDualPolarity = (DataTable[REG_DBG_DUAL_POLARITY] || (MeasurementType == MEASUREMENT_TYPE_AC)) ? TRUE : FALSE;

	SkipLoggingVoids = DataTable[REG_SKIP_LOGGING_VOIDS] ? TRUE : FALSE;

	// Optical connection monitor
	OptoConnectionMonMax = DataTable[REG_OPTO_CONNECTION_MON];

	// Select start voltage basing on measurement mode
	ControlledAmplitudeV = DesiredAmplitudeV = DesiredAmplitudeVHistory = _IQI(DataTable[REG_START_VOLTAGE_AC]);

	CurrentMultiply = 10;
	if (LimitCurrent <= HVD_IL_TH)
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

	if (LimitVoltage < HVD_VL_TH)
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

// No more.
