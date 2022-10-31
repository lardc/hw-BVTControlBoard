// -----------------------------------------
// Measuring logic DC
// ----------------------------------------


// Header
#include "MeasureDC.h"
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


// Types
//
typedef enum __DCProcessState
{
	DCPS_None = 0,
	DCPS_Ramp,
	DCPS_Plate,
	DCPS_Brake
} DCProcessState;


// Variables
//
static Int32U TimeCounter, PlateTimeCounter, PlateTimeCounterTop, PlateAvgSamplingStart, BrakeTimeCounter, BrakeTimeCounterTop;
static Int32U OptoConnectionMonMax;
static Int32U StepDCTimeCounter, StepDCTimeCounterTop;
static Int16U MeasurementType, OptoConnectionMon, CurrentMultiply, MaxPWM, FollowingErrorCounter;
static Int16S SSCurrentP2, SSVoltageP2;
static _iq SSCurrentCoff, SSCurrentP1, SSCurrentP0, SSVoltageCoff, SSVoltageP1, SSVoltageP0;
static _iq LimitCurrent, LimitVoltage;
static _iq KpVDC, KiVDC, SIDCerr;
static _iq ResultV, ResultI;
static Int16U ResultR, RFineK;
static Int32S CollectI, CollectV, CollectCounter, ResCurrentOffset;
static _iq DesiredVDC, VDCRateStep, StepDCVoltageStep, FollowingError;
static _iq MaxCurrent, MaxVoltage;
static DataSample ActualSecondarySample;
static Boolean TripCurrentDetected, UseInstantMethod;
static Boolean DbgSRAM, DbgMutePWM;
static Int16U Problem, Warning, Fault;
//
static volatile DCProcessState State = DCPS_None;


// Forward functions
//
static void MEASURE_DC_CacheVariables(_iq *OverrideLimitCurrent);
static void MEASURE_DC_ControlCycle();
static void MEASURE_DC_HandleVI();
static void MEASURE_DC_HandleTripCondition();


// Functions
//
Boolean MEASURE_DC_StartProcess(Int16U Type, pInt16U pDFReason, pInt16U pProblem, _iq *OverrideLimitCurrent)
{
	// Save parameters
	MeasurementType = Type;

	// Cache data
	MEASURE_DC_CacheVariables(OverrideLimitCurrent);

	// Enable RT cycle
	CONTROL_SwitchRTCycle(TRUE);

	// Init variables
	Problem = PROBLEM_NONE;
	Warning = WARNING_NONE;
	Fault = DF_NONE;
	OptoConnectionMon = 0;
	TripCurrentDetected = FALSE;
	TimeCounter = PlateTimeCounter = BrakeTimeCounter = 0;
	StepDCTimeCounter = 0;
	SIDCerr = 0;
	ResultR = ResultV = ResultI = 0;
	CollectI = CollectV = CollectCounter = 0;
	FollowingErrorCounter = 0;
	//
	ActualSecondarySample.IQFields.VoltageRaw = 0;
	ActualSecondarySample.IQFields.Voltage = 0;
	ActualSecondarySample.IQFields.Current = 0;
	MaxCurrent = MaxVoltage = 0;

	State = DCPS_Ramp;

	// Init buffers
	FIR_Reset();

	// Make proper commutation
	SS_Commutate(SwitchConfig_DC);
	// Reset PWM
	SS_SetPWM(0);
	// Configure samplers
	SS_ConfigureSensingCircuits(LimitCurrent, LimitVoltage, TRUE);
	// Start sampling
	SS_StartSampling();
	SS_Dummy(TRUE);
	DELAY_US(HV_SWITCH_DELAY);

	// Enable control cycle
	CONTROL_SubcribeToCycle(MEASURE_DC_ControlCycle);

	return TRUE;
}
// ----------------------------------------

void MEASURE_DC_FinishProcess()
{
	// Reset PWM
	SS_SetPWM(0);

	// Stop sampling
	SS_StopSampling();
	SS_Dummy(TRUE);

	CONTROL_SwitchRTCycle(FALSE);
}
// ----------------------------------------

void inline MEASURE_DC_SwitchToBrake()
{
	DesiredVDC = 0;
	BrakeTimeCounter = 0;
	State = DCPS_Brake;
}
// ----------------------------------------

Int16U inline MEASURE_DC_VoltageToPWM(_iq Voltage)
{
	// Recalculating formula
	Int32S pwm = _IQint(Voltage) / CTRL_VOLT_TO_PWM_DIV;

	if (pwm > MaxPWM)
		return MaxPWM;
	else if (pwm < 0)
		return 0;
	else
		return pwm;
}
// ----------------------------------------

void MEASURE_DC_Stop(Int16U Reason)
{
	if(Reason == DF_INTERNAL)
	{
		TripCurrentDetected = TRUE;
		MEASURE_DC_HandleTripCondition();
	}
	else if(Reason == DF_NONE)
		Problem = PROBLEM_STOP;
	else
		Fault = Reason;

	MEASURE_DC_SwitchToBrake();
}
// ----------------------------------------

void inline MEASURE_DC_DoSampling()
{
	_iq tmp, tmp2;
	_iq FilteredV, FilteredI;

	FIR_LoadValues(SS_Voltage, SS_Current);
	FIR_Apply(&FilteredV, &FilteredI);

	tmp = _IQmpy(SSVoltageCoff, SS_Voltage);
	tmp2 = _IQdiv(tmp, _IQ(1000.0f));
	ActualSecondarySample.IQFields.VoltageRaw = _IQmpy(tmp2, _IQmpyI32(tmp2, SSVoltageP2)) + _IQmpy(tmp, SSVoltageP1) + SSVoltageP0;

	tmp = _IQmpy(SSVoltageCoff, FilteredV);
	tmp2 = _IQdiv(tmp, _IQ(1000.0f));
	ActualSecondarySample.IQFields.Voltage = _IQmpy(tmp2, _IQmpyI32(tmp2, SSVoltageP2)) + _IQmpy(tmp, SSVoltageP1) + SSVoltageP0;

	tmp = _IQmpy(SSCurrentCoff, FilteredI);
	tmp2 = _IQdiv(tmp, _IQ(1000.0f));
	ActualSecondarySample.IQFields.Current = _IQmpy(tmp2, _IQmpyI32(tmp2, SSCurrentP2)) + _IQmpy(tmp, SSCurrentP1) + SSCurrentP0;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MEASURE_DC_HandleVI, "ramfuncs");
#endif
static void MEASURE_DC_HandleVI()
{
	// Connectivity monitoring
	if(OptoConnectionMonMax)
	{
		if(!SS_DataValid)
		{
			++OptoConnectionMon;
			if(OptoConnectionMon >= OptoConnectionMonMax)
				MEASURE_AC_Stop(DF_OPTO_CON_ERROR);
		}
		else
		{
			SS_DataValid = FALSE;
			OptoConnectionMon = 0;
		}
	}

	// Handle values
	switch (MeasurementType)
	{
		case MEASUREMENT_TYPE_DC:
		case MEASUREMENT_TYPE_DC_RES:
			{
				if (ActualSecondarySample.IQFields.Current > MaxCurrent)
					MaxCurrent = ActualSecondarySample.IQFields.Current;
			}
			break;

		case MEASUREMENT_TYPE_DC_STEP:
			switch (State)
			{
				case DCPS_Ramp:
					if ((ActualSecondarySample.IQFields.Current > MaxCurrent) && ((StepDCTimeCounter + 1) >= StepDCTimeCounterTop))
						MaxCurrent = ActualSecondarySample.IQFields.Current;
					break;

				case DCPS_Plate:
					if ((ActualSecondarySample.IQFields.Current > MaxCurrent) && PlateTimeCounter >= 10)
						MaxCurrent = ActualSecondarySample.IQFields.Current;
					break;
			}
			break;
	}

	// Average plate values
	if (State == DCPS_Plate && PlateTimeCounter > PlateAvgSamplingStart)
	{
		CollectV += _IQint(ActualSecondarySample.IQFields.Voltage);
		CollectI += _IQint(ActualSecondarySample.IQFields.Current);
		++CollectCounter;
	}

	// Detect maximum voltage for whole test cycle
	if (ActualSecondarySample.IQFields.Voltage > MaxVoltage)
		MaxVoltage = ActualSecondarySample.IQFields.Voltage;

	// Check current conditions
	if (MaxCurrent >= LimitCurrent)
		MEASURE_DC_Stop(DF_INTERNAL);
}
// ----------------------------------------

void inline MEASURE_DC_HandleTripCondition()
{
	if (MeasurementType == MEASUREMENT_TYPE_DC_RES)
	{
		ResultR = RES_LIMIT_LOW;
		Warning = WARNING_RES_OUT_OF_RANGE;
	}
	else
	{
		ResultI = ActualSecondarySample.IQFields.Current;
		ResultV = UseInstantMethod ? ActualSecondarySample.IQFields.Voltage : MaxVoltage;
	}
}
// ----------------------------------------

void MEASURE_DC_HandleNonTripCondition()
{
	if (MeasurementType == MEASUREMENT_TYPE_DC_RES)
	{
		CollectI += ResCurrentOffset * CollectCounter / 10;
		ResultR = (CollectV * 10) / CollectI;
		if (ResultR > (10 * RES_LIMIT_HIGH) || CollectI <= 0)
		{
			ResultR = RES_LIMIT_HIGH * 10;
			Warning = WARNING_RES_OUT_OF_RANGE;
		}
		else if (ResultR < (10 * RES_LIMIT_LOW))
		{
			ResultR = RES_LIMIT_LOW * 10;
			Warning = WARNING_RES_OUT_OF_RANGE;
		}
	}

	ResultV = _IQdiv(CollectV, CollectCounter);
	ResultI = _IQdiv(CollectI, CollectCounter);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MEASURE_DC_Regulator, "ramfuncs");
#endif
Int16U MEASURE_DC_Regulator(_iq DesiredVPrev, _iq DesiredVNew)
{
	if (!DbgMutePWM)
	{
		_iq err, p;
		_iq ControlledVDC;

		err = DesiredVPrev - ActualSecondarySample.IQFields.VoltageRaw;
		p = _IQmpy(err, KpVDC);
		SIDCerr += _IQsat(_IQmpy(err, KiVDC), _IQ((1L << (31 - GLOBAL_Q - 1)) - 1), _IQ(-(1L << (31 - GLOBAL_Q - 1))));
		SIDCerr =  _IQsat(SIDCerr, _IQ((1L << (31 - GLOBAL_Q - 1)) - 1), _IQ(-(1L << (31 - GLOBAL_Q - 1))));
		ControlledVDC = DesiredVNew + (SIDCerr + p);

		FollowingError = _IQdiv(err, DesiredVPrev);
		if (FollowingError > FE_DC_MAX_FRACTION && DesiredVPrev > FE_DC_MIN_VOLTAGE && DBG_USE_FOLLOWING_ERR)
		{
			if (FollowingErrorCounter++ > FE_DC_MAX_COUNTER)
			{
				ControlledVDC = 0;
				MEASURE_DC_Stop(DF_FOLLOWING_ERROR);
			}
		}
		else
			FollowingErrorCounter = 0;

		return MEASURE_DC_VoltageToPWM(ControlledVDC);
	}
	else
		return 0;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MEASURE_DC_ControlCycle, "ramfuncs");
#endif
static void MEASURE_DC_ControlCycle()
{
	_iq DesiredVDCPrev = DesiredVDC;
	Int16U correction;
	Int16S error;

	// Sample and handle values
	MEASURE_DC_DoSampling();
	if (State != DCPS_Brake) MEASURE_DC_HandleVI();

	// Setpoint generator
	switch(State)
	{
		case DCPS_Ramp:
			{
				switch (MeasurementType)
				{
					case MEASUREMENT_TYPE_DC:
					case MEASUREMENT_TYPE_DC_RES:
						{
							DesiredVDC += VDCRateStep;
							if (DesiredVDC >= LimitVoltage)
							{
								DesiredVDC = LimitVoltage;
								State = DCPS_Plate;
							}
						}
						break;
					case MEASUREMENT_TYPE_DC_STEP:
						{
							StepDCTimeCounter++;
							if (StepDCTimeCounter >= StepDCTimeCounterTop)
							{
								StepDCTimeCounter = 0;
								DesiredVDC += StepDCVoltageStep;
								if (DesiredVDC >= LimitVoltage)
								{
									DesiredVDC = LimitVoltage;
									State = DCPS_Plate;
								}
							}
						}
						break;
				}
			}
			break;
		case DCPS_Plate:
			{
				PlateTimeCounter++;
				if(PlateTimeCounter >= PlateTimeCounterTop)
					MEASURE_DC_SwitchToBrake();
			}
			break;
		case DCPS_Brake:
			{
				BrakeTimeCounter++;
				if(BrakeTimeCounter >= BrakeTimeCounterTop)
				{
					CONTROL_SubcribeToCycle(NULL);

					if(!TripCurrentDetected)
						MEASURE_DC_HandleNonTripCondition();

					DataTable[REG_RESULT_R] = (Int32U)ResultR * RFineK / 1000;
					CONTROL_NotifyEndTest(ResultV, _IQdiv(ResultI, _IQ(1000)), ResultI, Fault, Problem, Warning);
					State = DCPS_None;
				}
			}
			break;
	}

	// Apply correction
	correction = (State == DCPS_None || State == DCPS_Brake) ? 0 : MEASURE_DC_Regulator(DesiredVDCPrev, DesiredVDC);
	SS_SetPWM(correction);

	// Log data
	// Current and voltage
	MU_LogScopeIV(ActualSecondarySample);
	MU_LogScope(&ActualSecondarySample, CurrentMultiply, DbgSRAM, FALSE);
	// PWM output
	MU_LogScopeDIAG(correction);
	// Regulator error
	error = (State == DCPS_None || State == DCPS_Brake) ? 0 : _IQint(ActualSecondarySample.IQFields.Voltage - DesiredVDCPrev);
	MU_LogScopeErr(error);

	TimeCounter++;
}
// ----------------------------------------

static void MEASURE_DC_CacheVariables(_iq *OverrideLimitCurrent)
{
	switch (MeasurementType)
	{
		case MEASUREMENT_TYPE_DC:
			LimitVoltage = _IQI(DataTable[REG_DC_LIMIT_VOLTAGE]);
			VDCRateStep = _FPtoIQ2(DataTable[REG_DC_VOLTAGE_RATE] * 100, CONTROL_FREQUENCY);
			PlateTimeCounterTop = (CONTROL_FREQUENCY * DataTable[REG_DC_VOLTAGE_PLATE_TIME]) / 1000;
			LimitCurrent = _IQI(DataTable[REG_DC_LIMIT_CURRENT]);
			break;

		case MEASUREMENT_TYPE_DC_STEP:
			LimitVoltage = _IQI(DataTable[REG_DC_LIMIT_VOLTAGE]);
			StepDCTimeCounterTop = (CONTROL_FREQUENCY * DataTable[REG_DC_STEP_TIME]) / 1000;
			StepDCVoltageStep = _IQI(DataTable[REG_DC_STEP_VOLTAGE]);
			PlateTimeCounterTop = (CONTROL_FREQUENCY * DataTable[REG_DC_VOLTAGE_PLATE_TIME]) / 1000;
			LimitCurrent = _IQI(DataTable[REG_DC_LIMIT_CURRENT]);
			break;

		case MEASUREMENT_TYPE_DC_RES:
			LimitVoltage = _IQI(DataTable[REG_RES_VOLTAGE]);
			VDCRateStep = _FPtoIQ2(DataTable[REG_RES_VOLTAGE_RATE] * 100, CONTROL_FREQUENCY);
			PlateTimeCounterTop = (CONTROL_FREQUENCY * DC_RES_VPLATE) / 1000;
			LimitCurrent = OverrideLimitCurrent ? *OverrideLimitCurrent : RES_CURRENT_HIGH;
			ResCurrentOffset = (Int16S)(DataTable[REG_RES_CURR_OFFSET]);
			break;
	}

	// Calculate sampling period
	PlateAvgSamplingStart = (CONTROL_FREQUENCY * ((DataTable[REG_DC_VOLTAGE_PLATE_TIME] > DC_PLATE_SMPL_TIME) ?
							(DataTable[REG_DC_VOLTAGE_PLATE_TIME] - DC_PLATE_SMPL_TIME) : 0)) / 1000;

	KpVDC = _FPtoIQ2(DataTable[REG_KP_VDC_N], DataTable[REG_KP_VDC_D]);
	KiVDC = _FPtoIQ2(DataTable[REG_KI_VDC_N], DataTable[REG_KI_VDC_D]);
	BrakeTimeCounterTop = (CONTROL_FREQUENCY * DataTable[REG_BRAKE_TIME]) / 1000;
	MaxPWM = DataTable[REG_SAFE_MAX_PWM_DC];

	DbgSRAM = DataTable[REG_DBG_SRAM] ? TRUE : FALSE;
	DbgMutePWM = DataTable[REG_DBG_MUTE_PWM] ? TRUE : FALSE;

	DesiredVDC = 0;
	UseInstantMethod = DataTable[REG_USE_INST_METHOD] ? TRUE : FALSE;

	// Optical connection monitor
	OptoConnectionMonMax = DataTable[REG_OPTO_CONNECTION_MON];

	RFineK = DataTable[REG_R_RESULT_FINE_K];
	CurrentMultiply = 10;
	if (LimitCurrent <= HVD_DC_IL_TH)
	{
		SSCurrentCoff = _FPtoIQ2(DataTable[REG_SCURRENT_DCL_COFF_N], DataTable[REG_SCURRENT_DCL_COFF_D]);

		SSCurrentP2 = (Int16S)DataTable[REG_SCURRENT_DCL_FINE_P2];
		SSCurrentP1 = _FPtoIQ2(DataTable[REG_SCURRENT_DCL_FINE_P1], 1000);
		SSCurrentP0 = _FPtoIQ2((Int16S)DataTable[REG_SCURRENT_DCL_FINE_P0], 1000);
	}
	else
	{
		SSCurrentCoff = _FPtoIQ2(DataTable[REG_SCURRENT_DCM_COFF_N], DataTable[REG_SCURRENT_DCM_COFF_D]);

		SSCurrentP2 = (Int16S)DataTable[REG_SCURRENT_DCM_FINE_P2];
		SSCurrentP1 = _FPtoIQ2(DataTable[REG_SCURRENT_DCM_FINE_P1], 1000);
		SSCurrentP0 = _FPtoIQ2((Int16S)DataTable[REG_SCURRENT_DCM_FINE_P0], 1000);
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
