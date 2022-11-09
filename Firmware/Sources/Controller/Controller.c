// -----------------------------------------
// Controller logic
// ----------------------------------------

// Header
#include "Controller.h"
//
// Includes
#include "SysConfig.h"
#include "ZwDSP.h"
#include "ZbBoard.h"
#include "DataTable.h"
#include "SCCISlave.h"
#include "DeviceProfile.h"
#include "MemoryBuffers.h"
#include "IQmathUtils.h"
#include "PowerDriver.h"
#include "SelfTest.h"
#include "PrimarySampling.h"
#include "SecondarySampling.h"
#include "MeasureTest.h"
#include "MeasureUtils.h"


// Types
//
typedef void (*FUNC_AsyncDelegate)();
//
typedef struct __EndTestDPCClosure
{
	Boolean SavedRequestToDisable;
	Int16U SavedDFReason;
	Int16U SavedWarning;
	Int16U SavedProblem;
} EndTestDPCClosure;
//
typedef enum __BatteryVoltageState
{
	BVS_None		= 0,
	BVS_WaitRise	= 1,
	BVS_WaitFall	= 2,
	BVS_Ready		= 3
} BatteryVoltageState;


// Variables
//
volatile Int64U CONTROL_TimeCounter = 0;
volatile Int64U CONTROL_BatteryTimeout;
volatile DeviceState CONTROL_State = DS_None;
volatile BatteryVoltageState CONTROL_Battery = BVS_None;
static _iq ResCurrent;
static Int64U PulseToPulsePause;
static Boolean WaitSecondPulse;
//
static CONTROL_FUNC_RealTimeRoutine RealTimeRoutine = NULL;
static EndTestDPCClosure EndXDPCArgument = { FALSE, DF_NONE, WARNING_NONE, PROBLEM_NONE };
//
static volatile Boolean CycleActive = FALSE, BatteryVoltageIsReady;
static volatile FUNC_AsyncDelegate DPCDelegate = NULL;
static volatile Int16U CurrentMeasurementType = MEASUREMENT_TYPE_NONE;
//
// Boot-loader flag
#pragma DATA_SECTION(CONTROL_BootLoaderRequest, "bl_flag");
volatile Int16U CONTROL_BootLoaderRequest = 0;


// Forward functions
//
static void CONTROL_UpdateIdle();
static void CONTROL_FillWPPartDefault();
static void CONTROL_SetDeviceState(DeviceState NewState);
static void CONTROL_SwitchStateToPowered();
static void CONTROL_SwitchStateToInProcess();
static void CONTROL_SwitchStateToFault(Int16U FaultReason);
static void CONTROL_SwitchStateToDisabled(Int16U DisableReason);
static void CONTROL_TriggerMeasurementDPC();
static void CONTROL_EndTestDPC();
static void CONTROL_EndPassiveDPC();
static Boolean CONTROL_DispatchAction(Int16U ActionID, pInt16U UserError);
//
static void CONTROL_StartSequence();
static void CONTROL_BatteryVoltagePrepare();
static void CONTROL_BatteryVoltageConfig(Boolean DriverParam1, Boolean DriverParam2, BatteryVoltageState NewState);
static void CONTROL_BatteryVoltageCheck();
static void CONTROL_BatteryVoltageReady(Int16U Voltage);


// Functions
//
void CONTROL_Init()
{
	Int16U Fault = DF_NONE;

	Int16U EPIndexes[EP_COUNT] = { EP16_I, EP16_V, EP16_DIAG, EP16_ERR, EP16_PEAK_I, EP16_PEAK_V };
	Int16U EPSized[EP_COUNT] = { VALUES_x_SIZE, VALUES_x_SIZE, VALUES_x_SIZE, VALUES_x_SIZE, VALUES_x_SIZE, VALUES_x_SIZE };
	pInt16U EPCounters[EP_COUNT] = { (pInt16U)&MEMBUF_ValuesIV_Counter,		(pInt16U)&MEMBUF_ValuesIV_Counter,
									 (pInt16U)&MEMBUF_ValuesDIAG_Counter,	(pInt16U)&MEMBUF_ValuesErr_Counter,
									 (pInt16U)&MEMBUF_ValuesIVpeak_Counter,	(pInt16U)&MEMBUF_ValuesIVpeak_Counter };
	pInt16U EPDatas[EP_COUNT] = { MEMBUF_Values_I, MEMBUF_Values_V, MEMBUF_Values_DIAG,
								  MEMBUF_Values_Err, MEMBUF_Values_Ipeak, MEMBUF_Values_Vpeak };
	// Data-table EPROM service configuration
	EPROMServiceConfig EPROMService = { &ZbMemory_WriteValuesEPROM, &ZbMemory_ReadValuesEPROM };

	// Init data table
	DT_Init(EPROMService, FALSE);
	DT_SaveFirmwareInfo(DEVICE_CAN_ADDRESS, 0);

	// Fill state variables with default values
	CONTROL_SetDeviceState(DS_None);
	CONTROL_FillWPPartDefault();

	// Device profile initialization
	DEVPROFILE_Init(&CONTROL_DispatchAction, &CycleActive);
	DEVPROFILE_InitEPService(EPIndexes, EPSized, EPCounters, EPDatas);
	// Reset control values
	DEVPROFILE_ResetControlSection();

	// Calibrate ADC
	ZwADC_CalibrateLO(AIN_LO);

	// Enable driver
	DRIVER_Init();

	// Use quadratic correction for block
	DataTable[REG_QUADRATIC_CORR] = 1;

	// Check connections
	if(!ST_ValidateConnections(&Fault))
		CONTROL_SwitchStateToDisabled(Fault);

	if(ZwSystem_GetDogAlarmFlag())
	{
		DataTable[REG_WARNING] = WARNING_WATCHDOG_RESET;
		ZwSystem_ClearDogAlarmFlag();
	}
}
// ----------------------------------------

void CONTROL_DelayedInit()
{
	// Initialize sampling of parameters at primary side
	PSAMPLING_Init();
}
// ----------------------------------------

void inline CONTROL_RequestDPC(FUNC_AsyncDelegate Action)
{
	DPCDelegate = Action;
}
// ----------------------------------------

void CONTROL_Idle()
{
	CONTROL_UpdateIdle();
	DEVPROFILE_ProcessRequests();

	// Process second pulse
	if(WaitSecondPulse && !DPCDelegate && PulseToPulsePause < CONTROL_TimeCounter)
	{
		WaitSecondPulse = FALSE;
		CONTROL_RequestDPC(&CONTROL_StartSequence);
	}

	// Process deferred procedures
	if(DPCDelegate)
	{
		FUNC_AsyncDelegate del = DPCDelegate;
		DPCDelegate = NULL;
		del();
	}
}
// ----------------------------------------

void CONTROL_UpdateLow()
{
	// Update capacitor state
	PSAMPLING_DoSamplingVCap();
}
// ----------------------------------------

static void CONTROL_UpdateIdle()
{
	// Monitoring bridge temperature
	if(DBG_USE_TEMP_MON && DRIVER_ReadTemperatureFault())
	{
		if(CONTROL_State == DS_InProcess)
			CONTROL_RequestStop(DF_TEMP_MON, FALSE);

		if(CONTROL_State == DS_Powered)
			CONTROL_SwitchStateToFault(DF_TEMP_MON);
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(CONTROL_RealTimeCycle, "ramfuncs");
#endif
void CONTROL_RealTimeCycle()
{
	if(RealTimeRoutine)
		RealTimeRoutine();
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(CONTROL_SwitchRTCycle, "ramfuncs");
#endif
void CONTROL_SwitchRTCycle(Boolean Enable)
{
	if(Enable)
		ZwTimer_StartT0();
	else
		ZwTimer_StopT0();
}
// ----------------------------------------

void CONTROL_SubcribeToCycle(CONTROL_FUNC_RealTimeRoutine Routine)
{
	RealTimeRoutine = Routine;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(CONTROL_RequestStop, "ramfuncs");
#endif
void CONTROL_RequestStop(Int16U Reason, Boolean HWSignal)
{
	if(HWSignal)
		DRIVER_EnableTZandInt(FALSE);

	ZbGPIO_SwitchIndicator(FALSE);

	// Call stop process or switch to fault immediately
	if(CONTROL_State == DS_InProcess)
	{
		switch(CurrentMeasurementType)
		{
			case MEASUREMENT_TYPE_AC:
			case MEASUREMENT_TYPE_AC_R:
			case MEASUREMENT_TYPE_AC_D:
				MEASURE_AC_Stop(Reason);
				break;
			case MEASUREMENT_TYPE_DC:
			case MEASUREMENT_TYPE_DC_STEP:
			case MEASUREMENT_TYPE_DC_RES:
				MEASURE_DC_Stop(Reason);
				break;
			case MEASUREMENT_TYPE_TEST:
				MEASURE_TEST_Stop(Reason);
				break;
		}
	}
	else if(HWSignal)
	{
		EndXDPCArgument.SavedRequestToDisable = TRUE;
		EndXDPCArgument.SavedDFReason = Reason;

		CONTROL_RequestDPC(&CONTROL_EndPassiveDPC);
	}

	CurrentMeasurementType = MEASUREMENT_TYPE_NONE;

	DRIVER_ClearTZFault();
}
// ----------------------------------------

void CONTROL_NotifyEndTest(_iq BVTResultV, _iq BVTResultI, _iq BVTResultIuA, Int16U DFReason, Int16U Problem, Int16U Warning)
{
	if(BVTResultI < 0)
		BVTResultI = 0;

	if(CurrentMeasurementType == MEASUREMENT_TYPE_DC_RES && DFReason == DF_NONE && Problem == PROBLEM_NONE &&
			ResCurrent == RES_CURRENT_HIGH && _IQabs(BVTResultIuA) < RES_CURRENT_LOW)
	{
		ResCurrent = RES_CURRENT_LOW;
		PulseToPulsePause = CONTROL_TimeCounter + DC_RES_PULSE_TO_PULSE;
		WaitSecondPulse = TRUE;
		return;
	}

	DataTable[REG_RESULT_V] = _IQint(BVTResultV);
	DataTable[REG_RESULT_I] = _IQmpyI32int(BVTResultI, 10);
	DataTable[REG_RESULT_I_UA] = _IQint(BVTResultIuA) % 1000;
	DataTable[REG_RESULT_I_NA] = _IQmpyI32int(_IQfrac(BVTResultIuA), 1000);

	EndXDPCArgument.SavedDFReason = DFReason;
	EndXDPCArgument.SavedProblem = Problem;
	EndXDPCArgument.SavedWarning = Warning;

	CONTROL_RequestDPC(&CONTROL_EndTestDPC);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(CONTROL_NotifyCANFault, "ramfuncs");
#endif
void CONTROL_NotifyCANFault(ZwCAN_SysFlags Flag)
{
	DEVPROFILE_NotifyCANFault(Flag);
}
// ----------------------------------------

static void CONTROL_EndTestDPC()
{
	ZbGPIO_SwitchIndicator(FALSE);

	switch(CurrentMeasurementType)
	{
		case MEASUREMENT_TYPE_AC:
		case MEASUREMENT_TYPE_AC_R:
		case MEASUREMENT_TYPE_AC_D:
			MEASURE_AC_FinishProcess();
			break;
		case MEASUREMENT_TYPE_DC:
		case MEASUREMENT_TYPE_DC_STEP:
		case MEASUREMENT_TYPE_DC_RES:
			MEASURE_DC_FinishProcess();
			break;
		case MEASUREMENT_TYPE_TEST:
			MEASURE_TEST_FinishProcess();
			break;
	}

	if(EndXDPCArgument.SavedDFReason != DF_NONE)
	{
		if(EndXDPCArgument.SavedRequestToDisable)
			CONTROL_SwitchStateToDisabled(EndXDPCArgument.SavedDFReason);
		else
			CONTROL_SwitchStateToFault(EndXDPCArgument.SavedDFReason);
	}
	else
	{
		DataTable[REG_FINISHED] = (EndXDPCArgument.SavedProblem == PROBLEM_NONE) ? OPRESULT_OK : OPRESULT_FAIL;
		DataTable[REG_WARNING] = EndXDPCArgument.SavedWarning;
		DataTable[REG_PROBLEM] = (EndXDPCArgument.SavedProblem == PROBLEM_OUTPUT_SHORT) ? PROBLEM_NONE : EndXDPCArgument.SavedProblem;
		CONTROL_SwitchStateToPowered();
	}

	CurrentMeasurementType = MEASUREMENT_TYPE_NONE;

	CONTROL_ReInitSPI_Rx();
	DRIVER_ClearTZFault();
}
// ----------------------------------------

static void CONTROL_EndPassiveDPC()
{
	if(EndXDPCArgument.SavedRequestToDisable)
		CONTROL_SwitchStateToDisabled(EndXDPCArgument.SavedDFReason);
	else
		CONTROL_SwitchStateToFault(EndXDPCArgument.SavedDFReason);
}
// ----------------------------------------

static void CONTROL_FillWPPartDefault()
{
	DataTable[REG_FAULT_REASON] = DF_NONE;
	DataTable[REG_DISABLE_REASON] = DF_NONE;
	DataTable[REG_WARNING] = WARNING_NONE;
	DataTable[REG_PROBLEM] = PROBLEM_NONE;
	DataTable[REG_FINISHED] = OPRESULT_OK;
	DataTable[REG_RESULT_V] = 0;
	DataTable[REG_RESULT_I] = 0;
	DataTable[REG_RESULT_I_UA] = 0;
	DataTable[REG_RESULT_R] = 0;
}
// ----------------------------------------

static void CONTROL_SetDeviceState(DeviceState NewState)
{
	// Set new state
	CONTROL_State = NewState;
	DataTable[REG_DEV_STATE] = NewState;
}
// ----------------------------------------

static void CONTROL_SwitchStateToNone()
{
	// Switch off TZ interrupt
	DRIVER_EnableTZandInt(FALSE);
	// Switch off LED indicator
	ZbGPIO_SwitchIndicator(FALSE);
	// Switch off power supply
	DRIVER_SwitchPower(FALSE, FALSE);
	DataTable[REG_ACTUAL_PRIM_VOLTAGE] = 0;

	DataTable[REG_FAULT_REASON] = DF_NONE;
	CONTROL_SetDeviceState(DS_None);

	// Mark cycle inactive
	CycleActive = FALSE;
}
// ----------------------------------------

static void CONTROL_SwitchStateToPowered()
{
	// Enable power and protection signals
	DRIVER_EnableTZandInt(TRUE);
	// Configure monitor
	PSAMPLING_ConfigureSamplingVCap();
	CONTROL_SetDeviceState(DS_Powered);

	// Mark cycle inactive
	CycleActive = FALSE;
}
// ----------------------------------------

static void CONTROL_SwitchStateToInProcess()
{
	// Mark cycle active
	CycleActive = TRUE;

	CONTROL_SetDeviceState(DS_InProcess);
}
// ----------------------------------------

static void CONTROL_SwitchStateToFault(Int16U FaultReason)
{
	// Switch off TZ interrupt
	DRIVER_EnableTZandInt(FALSE);
	ZbGPIO_SwitchIndicator(FALSE);
	DRIVER_SwitchPower(FALSE, FALSE);

	DataTable[REG_FAULT_REASON] = FaultReason;
	CONTROL_SetDeviceState(DS_Fault);

	// Mark cycle inactive
	CycleActive = FALSE;
}
// ----------------------------------------

static void CONTROL_SwitchStateToDisabled(Int16U DisableReason)
{
	// Switch off TZ interrupt
	DRIVER_EnableTZandInt(FALSE);
	ZbGPIO_SwitchIndicator(FALSE);

	DataTable[REG_DISABLE_REASON] = DisableReason;
	CONTROL_SetDeviceState(DS_Disabled);

	// Mark cycle inactive
	CycleActive = FALSE;
}
// ----------------------------------------

static Boolean CONTROL_DispatchAction(Int16U ActionID, pInt16U UserError)
{
	switch(ActionID)
	{
		case ACT_ENABLE_POWER:
			{
				if(CONTROL_State == DS_None)
				{
					DRIVER_SwitchPower(TRUE, TRUE);
					CONTROL_SwitchStateToPowered();
				}
				else
					*UserError = ERR_DEVICE_NOT_READY;
			}
			break;
		case ACT_DISABLE_POWER:
			{
				if(CONTROL_State == DS_InProcess)
					*UserError = ERR_OPERATION_BLOCKED;
				else if(CONTROL_State == DS_Powered)
					CONTROL_SwitchStateToNone();
			}
			break;
		case ACT_START_TEST:
			{
				if(CONTROL_State == DS_InProcess)
				{
					*UserError = ERR_OPERATION_BLOCKED;
				}
				else if(CONTROL_State == DS_Powered)
				{
					CONTROL_FillWPPartDefault();
					DEVPROFILE_ResetScopes(0, IND_EP_I | IND_EP_V | IND_EP_DBG | IND_EP_ERR | IND_EP_PEAK_I | IND_EP_PEAK_V);
					DEVPROFILE_ResetEPReadState();
					
					ResCurrent = RES_CURRENT_HIGH;
					CONTROL_RequestDPC(&CONTROL_StartSequence);
				}
				else
					*UserError = ERR_DEVICE_NOT_READY;
			}
			break;
		case ACT_STOP:
			{
				if(CONTROL_State == DS_InProcess)
					CONTROL_RequestStop(DF_NONE, FALSE);
			}
			break;
		case ACT_READ_FRAGMENT:
			{
				DEVPROFILE_ResetScopes(0, IND_EP_I | IND_EP_V);
				DEVPROFILE_ResetEPReadState();

				if (DataTable[REG_USE_INST_METHOD] && DataTable[REG_REPLACE_CURVES])
				{
					MU_ReplaceIVbyPeakData();
					DEVPROFILE_ResetScopes(0, IND_EP_PEAK_I | IND_EP_PEAK_V);
				}
				else
					MU_LoadDataFragment();
			}
			break;
		case ACT_READ_MOVE_BACK:
			{
				MU_SeekScopeBack(DataTable[REG_DBG_READ_XY_FRAGMENT]);
			}
			break;
		case ACT_CLR_FAULT:
			{
				if(CONTROL_State == DS_Fault)
				{
					CONTROL_ReInitSPI_Rx();
					CONTROL_SwitchStateToNone();

					DataTable[REG_WARNING] = WARNING_NONE;
					DataTable[REG_FAULT_REASON] = DF_NONE;
				}
				else if(CONTROL_State == DS_Disabled)
					*UserError = ERR_OPERATION_BLOCKED;

			}
			break;
		case ACT_CLR_WARNING:
			DataTable[REG_WARNING] = WARNING_NONE;
			break;
		case ACT_DBG_PIN_PULSE:
			ZbGPIO_SwitchDebug(TRUE);
			DELAY_US(DataTable[REG_DBG_PULSE_LENGTH]);
			ZbGPIO_SwitchDebug(FALSE);
			break;
		case ACT_DBG_OPTO_DIGITIZER:
			if(CONTROL_State == DS_None)
				DataTable[REG_DIAG_PING_RESULT] = SS_Ping() ? 1 : 0;
			else
				*UserError = ERR_OPERATION_BLOCKED;
			break;
		case ACT_DBG_SET_PWM:
			if(CONTROL_State == DS_Powered)
				SS_SetPWM(DataTable[REG_DBG_SET_DC_PWM]);
			else
				*UserError = ERR_OPERATION_BLOCKED;
			break;
		default:
			return FALSE;
	}
	
	return TRUE;
}
// ----------------------------------------

void CONTROL_ReInitSPI_Rx()
{
	// Init master optical receiver interface
	ZwSPIa_Init(FALSE, 0, 16, SPIA_PLR, SPIA_PHASE, ZW_SPI_INIT_RX, TRUE, FALSE);
}
// -----------------------------------------

static void CONTROL_TriggerMeasurementDPC()
{
	Boolean success = FALSE;
	Int16U DFReason = DF_NONE, problem = PROBLEM_NONE;

	if(!(ResCurrent == RES_CURRENT_LOW && CurrentMeasurementType == MEASUREMENT_TYPE_DC_RES))
		MU_StartScope();

	// Re-init RX SPI channel and send dummy request
	CONTROL_ReInitSPI_Rx();
	SS_Dummy(FALSE);
	DELAY_US(10);

	switch(CurrentMeasurementType)
	{
		case MEASUREMENT_TYPE_TEST:
			success = MEASURE_TEST_StartProcess(CurrentMeasurementType, &DFReason, &problem);
			break;
		case MEASUREMENT_TYPE_AC:
		case MEASUREMENT_TYPE_AC_D:
		case MEASUREMENT_TYPE_AC_R:
			success = MEASURE_AC_StartProcess(CurrentMeasurementType, &DFReason, &problem);
			break;
		case MEASUREMENT_TYPE_DC:
		case MEASUREMENT_TYPE_DC_STEP:
			success = MEASURE_DC_StartProcess(CurrentMeasurementType, &DFReason, &problem, NULL);
			break;
		case MEASUREMENT_TYPE_DC_RES:
			success = MEASURE_DC_StartProcess(CurrentMeasurementType, &DFReason, &problem, &ResCurrent);
			break;
	}

	if(!success)
		CONTROL_NotifyEndTest(0, 0, 0, DFReason, problem, WARNING_NONE);
}
// ----------------------------------------

static void CONTROL_StartSequence()
{
	ZbGPIO_SwitchIndicator(TRUE);

	CurrentMeasurementType = DataTable[REG_MEASUREMENT_TYPE];
	DRIVER_ClearTZFault();
	CONTROL_SwitchStateToInProcess();

	CONTROL_BatteryVoltagePrepare();
}
// ----------------------------------------

static void CONTROL_BatteryVoltagePrepare()
{
	Int16U OutputPower = ((Int32U)DataTable[REG_LIMIT_VOLTAGE] * DataTable[REG_LIMIT_CURRENT]) / 10000;
	CONTROL_Battery = BVS_None;

	// For custom configured voltage
	if (DataTable[REG_USE_CUSTOM_PRIM_V])
	{
		CONTROL_BatteryVoltageReady(DataTable[REG_PRIM_V_CUSTOM]);
	}
	else
	{
		if ((OutputPower > CAP_SW_POWER) || (DataTable[REG_LIMIT_VOLTAGE] > CAP_SW_VOLTAGE) ||
			CurrentMeasurementType == MEASUREMENT_TYPE_DC ||
			CurrentMeasurementType == MEASUREMENT_TYPE_DC_STEP ||
			CurrentMeasurementType == MEASUREMENT_TYPE_DC_RES)
		{
			// Turn all power supplies in this case
			CONTROL_BatteryVoltageConfig(TRUE, TRUE, BVS_WaitRise);
		}
		else
		{
			// For low voltage operation
			if (DataTable[REG_ACTUAL_PRIM_VOLTAGE] > (DataTable[REG_PRIM_V_LOW_RANGE] + CAP_DELTA))
			{
				CONTROL_BatteryVoltageConfig(FALSE, FALSE, BVS_WaitFall);
			}
			else
			{
				DRIVER_SwitchPower(TRUE, FALSE);
				CONTROL_BatteryVoltageReady(DataTable[REG_ACTUAL_PRIM_VOLTAGE]);
			}
		}
	}
}
// ----------------------------------------

static void CONTROL_BatteryVoltageConfig(Boolean DriverParam1, Boolean DriverParam2, BatteryVoltageState NewState)
{
	DRIVER_SwitchPower(DriverParam1, DriverParam2);
	CONTROL_Battery = NewState;
	CONTROL_BatteryTimeout = CONTROL_TimeCounter + BAT_CHARGE_TIMEOUT;
	CONTROL_RequestDPC(&CONTROL_BatteryVoltageCheck);
}
// ----------------------------------------

static void CONTROL_BatteryVoltageCheck()
{
	Int16U FullRangeVoltage = (DataTable[REG_USE_CUSTOM_PRIM_V]) ? DataTable[REG_PRIM_V_CUSTOM] : DataTable[REG_PRIM_V_FULL_RANGE];

	switch (CONTROL_Battery)
	{
		case BVS_WaitRise:
			{
				if (DataTable[REG_ACTUAL_PRIM_VOLTAGE] >= (FullRangeVoltage - CAP_DELTA))
					CONTROL_BatteryVoltageReady(DataTable[REG_ACTUAL_PRIM_VOLTAGE]);
			}
			break;

		case BVS_WaitFall:
			{
				if (DataTable[REG_ACTUAL_PRIM_VOLTAGE] <= (DataTable[REG_PRIM_V_LOW_RANGE] + CAP_DELTA))
				{
					DRIVER_SwitchPower(TRUE, FALSE);
					CONTROL_BatteryVoltageReady(DataTable[REG_ACTUAL_PRIM_VOLTAGE]);
				}
			}
			break;
	}

	if (CONTROL_TimeCounter > CONTROL_BatteryTimeout)
		CONTROL_SwitchStateToFault(DF_LOW_SIDE_PS);
	else if (CONTROL_Battery != BVS_Ready)
		CONTROL_RequestDPC(&CONTROL_BatteryVoltageCheck);
}
// ----------------------------------------

static void CONTROL_BatteryVoltageReady(Int16U Voltage)
{
	CONTROL_Battery = BVS_Ready;
	DataTable[REG_PRIM_VOLTAGE_CTRL] = Voltage;
	CONTROL_RequestDPC(&CONTROL_TriggerMeasurementDPC);
}
// ----------------------------------------

// No more.
