// ----------------------------------------
// Logic controller
// ----------------------------------------

#ifndef __CONTROLLER_H
#define __CONTROLLER_H

// Include
#include "stdinc.h"
#include "ZwDSP.h"
#include "Global.h"
#include "DeviceObjectDictionary.h"
#include "MeasureAC.h"

// Types
typedef enum __DeviceState
{
	DS_None				= 0,
	DS_Fault			= 1,
	DS_Disabled			= 2,
	DS_Stopping			= 3,
	DS_Powered			= 4,
	DS_InProcess		= 5
} DeviceState;

typedef void (*CONTROL_FUNC_RealTimeRoutine)();

// Variables
extern volatile Int64U CONTROL_TimeCounter;
extern volatile DeviceState CONTROL_State;
extern volatile Int16U CONTROL_BootLoaderRequest;

extern Int16U MEMBUF_Values_V[];
extern Int16U MEMBUF_Values_ImA[];
extern Int16U MEMBUF_Values_IuA[];
extern Int16U MEMBUF_Values_Vrms[];
extern Int16U MEMBUF_Values_Irms_mA[];
extern Int16U MEMBUF_Values_Irms_uA[];
extern Int16U MEMBUF_Values_PWM[];
extern Int16U MEMBUF_Values_Err[];

extern volatile Int16U MEMBUF_ScopeValues_Counter;
extern volatile Int16U MEMBUF_ErrorValues_Counter;

// Functions
// Initialize controller
void CONTROL_Init();
// Do background idle operation
void CONTROL_Idle();
// Real-time control routine
void CONTROL_RealTimeCycle();
// Switch-on/off real-time cycle
void CONTROL_SwitchRTCycle(Boolean Enable);
// Subscribe to real-time cycle
void CONTROL_SubcribeToCycle(CONTROL_FUNC_RealTimeRoutine Routine);
// Emergency stop process
void CONTROL_RequestStop(Int16U Reason, Boolean HWSignal);
// Set test result
void CONTROL_NotifyEndTest(_iq BVTResultV, _iq BVTResultI, Int16U DFReason, Int16U Problem, Int16U Warning);
// Notify that CAN system fault occurs
void CONTROL_NotifyCANFault(ZwCAN_SysFlags Flag);
// Re-init RX SPI channels
void CONTROL_ReInitSPI_Rx();

#endif // __CONTROLLER_H
