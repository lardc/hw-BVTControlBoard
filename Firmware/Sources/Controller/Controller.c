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
Int16U MEMBUF_Values_Vrms[VALUES_x_SIZE];
Int16U MEMBUF_Values_Irms_mA[VALUES_x_SIZE];
Int16U MEMBUF_Values_Irms_uA[VALUES_x_SIZE];
Int16U MEMBUF_Values_PWM[VALUES_x_SIZE];
Int16U MEMBUF_Values_CosPhi[VALUES_x_SIZE];
Int16U MEMBUF_Values_Err[VALUES_x_SIZE];
//
volatile Int16U MEMBUF_ScopeValues_Counter = 0;
volatile Int16U MEMBUF_ErrorValues_Counter = 0;
//
static Int16U PrimaryVoltage;
volatile Int64U CONTROL_TimeCounter = 0;
volatile DeviceState CONTROL_State = DS_None;
volatile Boolean CONTROL_DataReceiveAck = FALSE;
//
static CONTROL_FUNC_RealTimeRoutine RealTimeRoutine = NULL;
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
	Int16U EPIndexes[EP_COUNT] = {EP16_V, EP16_ImA, EP16_IuA, EP16_Vrms, EP16_Irms_mA, EP16_Irms_uA,
			EP16_PWM, EP16_CosPhi, EP16_Error};
	Int16U EPSized[EP_COUNT] = {VALUES_x_SIZE, VALUES_x_SIZE, VALUES_x_SIZE, VALUES_x_SIZE,
			VALUES_x_SIZE, VALUES_x_SIZE, VALUES_x_SIZE, VALUES_x_SIZE, VALUES_x_SIZE};
	pInt16U cnt = (pInt16U)&MEMBUF_ScopeValues_Counter;
	pInt16U EPCounters[EP_COUNT] =
			{cnt, cnt, cnt, cnt, cnt, cnt, cnt, cnt, (pInt16U)&MEMBUF_ErrorValues_Counter};
	pInt16U EPDatas[EP_COUNT] = {MEMBUF_Values_V, MEMBUF_Values_ImA, MEMBUF_Values_IuA,
			MEMBUF_Values_Vrms, MEMBUF_Values_Irms_mA, MEMBUF_Values_Irms_uA, MEMBUF_Values_PWM,
			MEMBUF_Values_CosPhi, MEMBUF_Values_Err};

	// Data-table EPROM service configuration
	EPROMServiceConfig EPROMService = {&ZbMemory_WriteValuesEPROM, &ZbMemory_ReadValuesEPROM};

	// Init data table
	DT_Init(EPROMService, FALSE);
	DT_SaveFirmwareInfo(DEVICE_CAN_ADDRESS, 0);

	// Fill state variables with default values
	CONTROL_ResetValues();

	// Device profile initialization
	static Boolean MaskChanges = FALSE;
	DEVPROFILE_Init(&CONTROL_DispatchAction, &MaskChanges);
	DEVPROFILE_InitEPService(EPIndexes, EPSized, EPCounters, EPDatas);
	// Reset control values
	DEVPROFILE_ResetControlSection();

	PS_Init();
	SS_Ping();

	// Use quadratic correction for block
	DataTable[REG_QUADRATIC_CORR] = 1;
	DataTable[REG_I_LIMIT_RANGE1] = _IQint(I_RANGE_HIGH);
	DataTable[REG_I_LIMIT_RANGE2] = _IQint(I_RANGE_MID);
	DataTable[REG_I_LIMIT_RANGE3] = _IQint(I_RANGE_LOW);

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
	DataTable[REG_ACTUAL_PRIM_VOLTAGE] = PrimaryVoltage = PS_GetBatteryVoltage();
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(CONTROL_DataRequestRoutine, "ramfuncs");
#endif
void CONTROL_DataRequestRoutine()
{
	if(CONTROL_DataReceiveAck)
	{
		CONTROL_DataReceiveAck = FALSE;
		SS_GetData(FALSE);
		CONTROL_RealTimeACRoutine();
	}
	else
	{
		CONTROL_SubcribeToCycle(NULL);
		CONTROL_SwitchRTCycle(FALSE);
		CONTROL_SwitchStateToFault(DF_OPTICAL_INTERFACE);
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(CONTROL_RealTimeACRoutine, "ramfuncs");
#endif
void CONTROL_RealTimeACRoutine()
{
	if(RealTimeRoutine)
		RealTimeRoutine();
}
// ----------------------------------------

void CONTROL_SwitchRTCycle(Boolean Enable)
{
	if(Enable)
	{
		CONTROL_DataReceiveAck = TRUE;
		ZwTimer_StartT0();
	}
	else
	{
		ZwPWMB_SetValue12(0);
		ZwTimer_StopT0();
	}
}
// ----------------------------------------

void CONTROL_SubcribeToCycle(CONTROL_FUNC_RealTimeRoutine Routine)
{
	RealTimeRoutine = Routine;
}
// ----------------------------------------

void CONTROL_RequestStop()
{
	CONTROL_SetDeviceState(DS_Powered);
}
// ----------------------------------------

void CONTROL_NotifyCANFault(ZwCAN_SysFlags Flag)
{
	DEVPROFILE_NotifyCANFault(Flag);
}
// ----------------------------------------

void CONTROL_ResetValues()
{
	DataTable[REG_FAULT_REASON] = DF_NONE;
	DataTable[REG_DISABLE_REASON] = DF_NONE;
	DataTable[REG_WARNING] = WARNING_NONE;
	DataTable[REG_PROBLEM] = PROBLEM_NONE;
	DataTable[REG_FINISHED] = OPRESULT_NONE;
	DataTable[REG_VOLTAGE_READY] = 0;
	DataTable[REG_RESULT_V] = 0;
	DataTable[REG_RESULT_I_mA] = 0;
	DataTable[REG_RESULT_I_uA] = 0;
	DataTable[REG_RESULT_I_ACT_mA] = 0;
	DataTable[REG_RESULT_I_ACT_uA] = 0;
	DataTable[REG_RESULT_COS_PHI] = 0;
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
			if(CONTROL_State == DS_None)
				CONTROL_SetDeviceState(DS_Powered);
			else if(CONTROL_State != DS_Powered)
				*UserError = ERR_DEVICE_NOT_READY;
			break;

		case ACT_DISABLE_POWER:
			if(CONTROL_State == DS_InProcess)
				*UserError = ERR_OPERATION_BLOCKED;
			else
				CONTROL_SetDeviceState(DS_None);
			break;

		case ACT_START_TEST:
			if(CONTROL_State == DS_InProcess)
				*UserError = ERR_OPERATION_BLOCKED;
			else if(CONTROL_State == DS_Powered)
			{
				CONTROL_ResetValues();
				DEVPROFILE_ResetScopes(0);
				DEVPROFILE_ResetEPReadState();

				CONTROL_StartSequence();
			}
			else
				*UserError = ERR_DEVICE_NOT_READY;
			break;

		case ACT_STOP:
			if(CONTROL_State == DS_InProcess)
				MAC_RequestStop(PBR_RequestStop);
			break;

		case ACT_READ_FRAGMENT:
			{
				Int16U i;
				for(i = EP16_V; i <= EP16_CosPhi; i++)
					DEVPROFILE_ResetScopes(i);
				DEVPROFILE_ResetEPReadState();
				MU_LoadDataFragment();
			}
			break;

		case ACT_CLR_FAULT:
			if(CONTROL_State == DS_Fault)
			{
				CONTROL_SetDeviceState(DS_None);
				DataTable[REG_FAULT_REASON] = DF_NONE;
			}
			break;

		case ACT_CLR_WARNING:
			DataTable[REG_WARNING] = WARNING_NONE;
			break;

		case ACT_DBG_DIGI_GET_PACKET:
			SS_GetLastMessage();
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
	// Проверка напряжения первичной стороны
	Int16U Delta = ABS((Int32S)PrimaryVoltage - DataTable[REG_PRIM_VOLTAGE]) * 100 / DataTable[REG_PRIM_VOLTAGE];
	if(Delta <= PRIM_V_MAX_DELTA || DataTable[REG_PRIM_IGNORE_CHECK])
	{
		MU_StartScope();
		if(MAC_StartProcess())
			CONTROL_SetDeviceState(DS_InProcess);
		else
			CONTROL_SwitchStateToFault(DF_OPTICAL_INTERFACE);
	}
	else
		CONTROL_SwitchStateToFault(DF_PRIMARY_VOLTAGE);
}
// ----------------------------------------
