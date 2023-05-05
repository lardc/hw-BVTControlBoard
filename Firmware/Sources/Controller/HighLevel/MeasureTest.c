// ----------------------------------------
// Measuring logic test
// ----------------------------------------

// Header
#include "MeasureTest.h"
//
// Includes
#include "SysConfig.h"
#include "ZwDSP.h"
#include "ZbBoard.h"
#include "DataTable.h"
#include "DeviceObjectDictionary.h"
#include "IQmathUtils.h"
#include "Global.h"
#include "Controller.h"
#include "PrimarySampling.h"
#include "MeasureUtils.h"
#include "SecondarySampling.h"
#include "PowerDriver.h"

// Types
//
typedef enum __TestProcessState
{
	TPS_None = 0,
	TPS_Plate,
	TPS_Brake
} TestProcessState;


// Variables
//
static Int32U TimeCounter, BrakeCounter, BrakeCounterMax;
static Int16S MinSafePWM, TestAmplitude, ActualSecondaryCurrent = 0, ActualSecondaryVoltage = 0;
static _iq NormalizedFrequency, LimitVoltage, LimitCurrent;
static Int16U Problem, Warning, Fault;
static Boolean ConstPWMMode;
//
static volatile TestProcessState State = TPS_None;


// Forward functions
//
static void MEASURE_TEST_CacheVariables();
static void MEASURE_TEST_ControlCycle();


// Functions
//
Boolean MEASURE_TEST_StartProcess(Int16U Type, pInt16U pDFReason, pInt16U pProblem)
{
	// Cache data
	MEASURE_TEST_CacheVariables();
	// Enable RT cycle
	CONTROL_SwitchRTCycle(TRUE);

	// Init variables
	Problem = PROBLEM_NONE;
	Warning = WARNING_NONE;
	Fault = DF_NONE;
	TimeCounter = 0;
	State = TPS_Plate;

	// Configure samplers
	SS_ConfigureSensingCircuits(LimitCurrent, LimitVoltage);
	// Start sampling
	SS_StartSampling();
	SS_Dummy(TRUE);

	// Enable control cycle
	CONTROL_SubcribeToCycle(&MEASURE_TEST_ControlCycle);

	// Enable PWM
	ZwPWMB_SetValue12(0);
	ZwPWM_Enable(TRUE);

	return TRUE;
}
// ----------------------------------------

void inline MEASURE_TEST_SetPWM(Int16S Duty)
{
	if(ABS(Duty) < (MinSafePWM / 2))
		ZwPWMB_SetValue12(0);
	else if(ABS(Duty) < MinSafePWM)
		ZwPWMB_SetValue12(MinSafePWM * SIGN(Duty));
	else
		ZwPWMB_SetValue12(Duty);
}
// ----------------------------------------

void MEASURE_TEST_FinishProcess()
{
	SS_StopSampling();
	SS_Dummy(TRUE);

	CONTROL_SwitchRTCycle(FALSE);
}
// ----------------------------------------

void inline MEASURE_TEST_SwitchToBrake()
{
	DINT;
	ZwPWMB_SetValue12(0);
	DELAY_US((1000000L / PWM_FREQUENCY) * 3);
	ZwPWM_Enable(FALSE);
	EINT;

	BrakeCounter = 0;
	State = TPS_Brake;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MEASURE_TEST_Stop, "ramfuncs");
#endif
void MEASURE_TEST_Stop(Int16U Reason)
{
	if(Reason == DF_NONE)
		Problem = PROBLEM_STOP;
	else if(Reason != DF_INTERNAL)
		Fault = Reason;

	MEASURE_TEST_SwitchToBrake();
}
// ----------------------------------------

static inline MEASURE_TEST_DoSampling()
{
	SS_DoSampling();

	ActualSecondaryCurrent = SS_Current;
	ActualSecondaryVoltage = SS_Voltage;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MEASURE_TEST_ControlCycle, "ramfuncs");
#endif
static void MEASURE_TEST_ControlCycle()
{
	Int16S desiredPWMValue = 0;

	MEASURE_TEST_DoSampling();

	TimeCounter++;
	switch(State)
	{
		case TPS_Plate:
			{
				desiredPWMValue = _IQmpyI32int(_IQsinPU(_IQmpyI32(NormalizedFrequency, TimeCounter)), TestAmplitude);

				if(ConstPWMMode)
					desiredPWMValue = TestAmplitude;

				MEASURE_TEST_SetPWM(desiredPWMValue);

				MU_LogScopeRaw(ActualSecondaryVoltage, ActualSecondaryCurrent, FALSE);
				MU_LogScopeDIAG(desiredPWMValue);
			}
			break;
		case TPS_Brake:
			{
				BrakeCounter++;

				if(BrakeCounter >= BrakeCounterMax)
				{
					CONTROL_SubcribeToCycle(NULL);
					CONTROL_NotifyEndTest(0, 0, Fault, Problem, Warning);

					State = TPS_None;
				}
			}
			break;
	}
}
// ----------------------------------------

static void MEASURE_TEST_CacheVariables()
{
	LimitCurrent = _FPtoIQ2(DataTable[REG_TEST_CURRENT], 10);
	LimitVoltage = _IQI(DataTable[REG_TEST_VOLTAGE]);

	TestAmplitude = DataTable[REG_TEST_PWM_AMPLITUDE];
	NormalizedFrequency = _IQdiv(_IQ(1.0f), _IQI(CONTROL_FREQUENCY / DataTable[REG_TEST_FREQUENCY]));
	BrakeCounterMax = (CONTROL_FREQUENCY * DataTable[REG_BRAKE_TIME]) / 1000;
	ConstPWMMode = DataTable[REG_TEST_CONST_PWM_MODE] ? TRUE : FALSE;
	MinSafePWM = (PWM_FREQUENCY / 1000L) * PWM_TH * ZW_PWM_DUTY_BASE / 1000000L;
}
// ----------------------------------------

// No more.
