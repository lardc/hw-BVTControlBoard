// ----------------------------------------
// Controller logic
// ----------------------------------------

// Header
#include "Controller.h"

// Includes
#include "SysConfig.h"
#include "ZwDSP.h"
#include "ZbBoard.h"
#include "DataTable.h"
#include "SCCISlave.h"
#include "DeviceProfile.h"
#include "IQmathUtils.h"
#include "PrimarySampling.h"
#include "SecondarySampling.h"
#include "MeasureUtils.h"
#include "DataTable.h"
#include "DeviceObjectDictionary.h"

// Types
//
typedef void (*FUNC_AsyncDelegate)();
//
typedef struct __EndTestDPCClosure
{
	Boolean SavedRequestToDisable;
	Int16S SavedResultV;
	Int16S SavedResultI;
	Int16U SavedDFReason;
	Int16U SavedWarning;
	Int16U SavedProblem;
} EndTestDPCClosure;

// Variables
//
Int16U MEMBUF_Values_V[VALUES_x_SIZE];
Int16U MEMBUF_Values_ImA[VALUES_x_SIZE];
Int16U MEMBUF_Values_IuA[VALUES_x_SIZE];
Int16U MEMBUF_Values_PWM[VALUES_x_SIZE];
Int16U MEMBUF_Values_Err[VALUES_x_SIZE];
//
volatile Int16U MEMBUF_ValuesVI_Counter = 0;
volatile Int16U MEMBUF_ValuesPWM_Counter = 0;
volatile Int16U MEMBUF_ValuesErr_Counter = 0;
//
volatile Int64U CONTROL_TimeCounter = 0;
volatile DeviceState CONTROL_State = DS_None;
//
static CONTROL_FUNC_RealTimeRoutine RealTimeRoutine = NULL;
static EndTestDPCClosure EndXDPCArgument = { FALSE, 0, 0, DF_NONE, WARNING_NONE, PROBLEM_NONE };
//
static volatile FUNC_AsyncDelegate DPCDelegate = NULL;
//
// Boot-loader flag
#pragma DATA_SECTION(CONTROL_BootLoaderRequest, "bl_flag");
volatile Int16U CONTROL_BootLoaderRequest = 0;

// Forward functions
//
void CONTROL_ResetValues();
void CONTROL_SetDeviceState(DeviceState NewState);
void CONTROL_SwitchStateToFault(Int16U FaultReason);
void CONTROL_EndTestDPC();
void CONTROL_EndPassiveDPC();
Boolean CONTROL_DispatchAction(Int16U ActionID, pInt16U UserError);
void CONTROL_StartSequence();

// Functions
//
void CONTROL_Init()
{
	Int16U EPIndexes[EP_COUNT] = {EP16_V, EP16_ImA, EP16_IuA, EP16_PWM, EP16_Error};
	Int16U EPSized[EP_COUNT] = {VALUES_x_SIZE, VALUES_x_SIZE, VALUES_x_SIZE, VALUES_x_SIZE, VALUES_x_SIZE};
	pInt16U pVIcnt = (pInt16U)&MEMBUF_ValuesVI_Counter;
	pInt16U EPCounters[EP_COUNT] =
			{pVIcnt, pVIcnt, pVIcnt, (pInt16U)&MEMBUF_ValuesPWM_Counter, (pInt16U)&MEMBUF_ValuesErr_Counter};
	pInt16U EPDatas[EP_COUNT] =
			{MEMBUF_Values_V, MEMBUF_Values_ImA, MEMBUF_Values_IuA, MEMBUF_Values_PWM, MEMBUF_Values_Err};

	// Data-table EPROM service configuration
	EPROMServiceConfig EPROMService = {&ZbMemory_WriteValuesEPROM, &ZbMemory_ReadValuesEPROM};

	// Init data table
	DT_Init(EPROMService, FALSE);
	DT_SaveFirmwareInfo(DEVICE_CAN_ADDRESS, 0);

	// Fill state variables with default values
	CONTROL_ResetValues();

	// Device profile initialization
	Boolean MaskChanges = FALSE;
	DEVPROFILE_Init(&CONTROL_DispatchAction, &MaskChanges);
	DEVPROFILE_InitEPService(EPIndexes, EPSized, EPCounters, EPDatas);
	// Reset control values
	DEVPROFILE_ResetControlSection();

	PS_Init();
	SS_Ping();

	// Use quadratic correction for block
	DataTable[REG_QUADRATIC_CORR] = 1;

	if(ZwSystem_GetDogAlarmFlag())
	{
		DataTable[REG_WARNING] = WARNING_WATCHDOG_RESET;
		ZwSystem_ClearDogAlarmFlag();
	}
}
// ----------------------------------------

void CONTROL_Idle()
{
	DEVPROFILE_ProcessRequests();
	DataTable[REG_ACTUAL_PRIM_VOLTAGE] = PS_GetBatteryVoltage();

	// Process deferred procedures
	if(DPCDelegate)
	{
		FUNC_AsyncDelegate del = DPCDelegate;
		DPCDelegate = NULL;
		del();
	}
}
// ----------------------------------------

void inline CONTROL_RequestDPC(FUNC_AsyncDelegate Action)
{
	DPCDelegate = Action;
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
	if(CONTROL_State == DS_InProcess)
	{
		MAC_Stop(Reason);
	}
	else if(HWSignal)
	{
		EndXDPCArgument.SavedRequestToDisable = TRUE;
		EndXDPCArgument.SavedDFReason = Reason;

		CONTROL_RequestDPC(&CONTROL_EndPassiveDPC);
	}
}
// ----------------------------------------

void CONTROL_NotifyEndTest(_iq BVTResultV, _iq BVTResultI, Int16U DFReason, Int16U Problem, Int16U Warning)
{
	// Save values for further processing
	EndXDPCArgument.SavedResultV = _IQint(BVTResultV);
	EndXDPCArgument.SavedResultI = _IQmpyI32int(BVTResultI, 10);
	DataTable[REG_RESULT_I_UA_R] = (BVTResultI > 0) ? _IQmpyI32int(_IQfrac(BVTResultI), 1000) : 0;
	//
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

void CONTROL_EndTestDPC()
{
	MAC_FinishProcess();

	if(EndXDPCArgument.SavedDFReason != DF_NONE)
		CONTROL_SwitchStateToFault(EndXDPCArgument.SavedDFReason);
	else
	{
		DataTable[REG_FINISHED] = (EndXDPCArgument.SavedProblem == PROBLEM_NONE) ? OPRESULT_OK : OPRESULT_FAIL;
		DataTable[REG_WARNING] = EndXDPCArgument.SavedWarning;
		DataTable[REG_PROBLEM] = (EndXDPCArgument.SavedProblem == PROBLEM_OUTPUT_SHORT) ? PROBLEM_NONE : EndXDPCArgument.SavedProblem;
		DataTable[REG_RESULT_V] = EndXDPCArgument.SavedResultV;
		DataTable[REG_RESULT_I] = EndXDPCArgument.SavedResultI;
		CONTROL_SetDeviceState(DS_Powered);
	}

	CONTROL_ReInitSPI_Rx();
}
// ----------------------------------------

void CONTROL_EndPassiveDPC()
{
	if(EndXDPCArgument.SavedRequestToDisable)
		CONTROL_SwitchStateToFault(EndXDPCArgument.SavedDFReason);
}
// ----------------------------------------

void CONTROL_ResetValues()
{
	// Set volatile states
	DataTable[REG_DEV_STATE] = (Int16U)DS_None;
	DataTable[REG_FAULT_REASON] = DF_NONE;
	DataTable[REG_DISABLE_REASON] = DF_NONE;
	DataTable[REG_WARNING] = WARNING_NONE;
	DataTable[REG_PROBLEM] = PROBLEM_NONE;
	DataTable[REG_FINISHED] = OPRESULT_OK;
	DataTable[REG_RESULT_V] = 0;
	DataTable[REG_RESULT_I] = 0;
	DataTable[REG_RESULT_I_UA_R] = 0;
	//
	DataTable[REG_ACTUAL_PRIM_VOLTAGE] = 0;
}
// ----------------------------------------

void CONTROL_SetDeviceState(DeviceState NewState)
{
	ZbGPIO_SwitchIndicator(NewState == DS_InProcess);

	CONTROL_State = NewState;
	DataTable[REG_DEV_STATE] = NewState;
}
// ----------------------------------------

void CONTROL_SwitchStateToFault(Int16U FaultReason)
{
	DataTable[REG_FAULT_REASON] = FaultReason;
	CONTROL_SetDeviceState(DS_Fault);
}
// ----------------------------------------

Boolean CONTROL_DispatchAction(Int16U ActionID, pInt16U UserError)
{
	switch(ActionID)
	{
		case ACT_ENABLE_POWER:
			{
				if(CONTROL_State == DS_None)
				{
					CONTROL_SetDeviceState(DS_Powered);
				}
				else
					*UserError = ERR_DEVICE_NOT_READY;
			}
			break;

		case ACT_DISABLE_POWER:
			{
				if(CONTROL_State == DS_InProcess || CONTROL_State == DS_Stopping)
					*UserError = ERR_OPERATION_BLOCKED;
				else if(CONTROL_State == DS_Powered)
					CONTROL_SetDeviceState(DS_None);
			}
			break;

		case ACT_START_TEST:
			{
				if(CONTROL_State == DS_InProcess || CONTROL_State == DS_Stopping)
				{
					*UserError = ERR_OPERATION_BLOCKED;
				}
				else if(CONTROL_State == DS_Powered)
				{
					DataTable[REG_FINISHED] = OPRESULT_NONE;
					DataTable[REG_PROBLEM] = PROBLEM_NONE;
					DataTable[REG_WARNING] = WARNING_NONE;
					DataTable[REG_RESULT_V] = 0;
					DataTable[REG_RESULT_I] = 0;
					DataTable[REG_RESULT_I_UA_R] = 0;
					DEVPROFILE_ResetScopes(0);
					DEVPROFILE_ResetEPReadState();

					CONTROL_StartSequence();
				}
				else
					*UserError = ERR_DEVICE_NOT_READY;
			}
			break;

		case ACT_STOP:
			{
				if(CONTROL_State == DS_InProcess)
				{
					CONTROL_RequestDPC(NULL);
					CONTROL_SetDeviceState(DS_Powered);
				}
			}
			break;

		case ACT_READ_FRAGMENT:
			{
				DEVPROFILE_ResetScopes(EP16_V);
				DEVPROFILE_ResetScopes(EP16_ImA);
				DEVPROFILE_ResetScopes(EP16_IuA);
				DEVPROFILE_ResetEPReadState();
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
					CONTROL_SetDeviceState(DS_None);

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

		case ACT_DBG_DIGI_GET_PACKET:
			if(CONTROL_State == DS_None)
				SS_GetLastMessage();
			else
				*UserError = ERR_OPERATION_BLOCKED;
			break;

		case ACT_DBG_DIGI_PING:
			if(CONTROL_State == DS_None)
				DataTable[REG_DIAG_DIGI_RESULT] = SS_Ping() ? 1 : 0;
			else
				*UserError = ERR_OPERATION_BLOCKED;
			break;

		case ACT_DBG_DIGI_SAMPLE:
			if(CONTROL_State == DS_None)
				DataTable[REG_DIAG_DIGI_RESULT] = SS_GetData(TRUE) ? 1 : 0;
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
	ZwSPIa_Init(FALSE, 0, IBP_CHAR_SIZE, SPIA_PLR, SPIA_PHASE, ZW_SPI_INIT_RX, TRUE, FALSE);
}
// -----------------------------------------

void CONTROL_StartSequence()
{
	MU_StartScope();
	if(MAC_StartProcess())
		CONTROL_SetDeviceState(DS_InProcess);
	else
		CONTROL_SwitchStateToFault(DF_OPTO_CON_ERROR);
}
// ----------------------------------------
