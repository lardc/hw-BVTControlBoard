// ----------------------------------------
// Logic controller
// ----------------------------------------

#ifndef __CONTROLLER_H
#define __CONTROLLER_H

// Include
#include "stdinc.h"
//
#include "ZwDSP.h"
#include "Global.h"
#include "DeviceObjectDictionary.h"
#include "MeasureAC.h"


// Types
//
typedef enum __DeviceState
{
	DS_None				= 0,
	DS_Fault			= 1,
	DS_Disabled			= 2,
	DS_Stopping			= 3,
	DS_Powered			= 4,
	DS_InProcess		= 5
} DeviceState;
//
typedef void (*CONTROL_FUNC_RealTimeRoutine)();

// Variables
//
extern volatile Int64U CONTROL_TimeCounter;
extern Int16U CONTROL_ExtInfoCounter;
extern volatile DeviceState CONTROL_State;
extern volatile Int16U CONTROL_BootLoaderRequest;


// Functions
//
// Initialize controller
void CONTROL_Init();
// Delayed initialization routine
void CONTROL_DelayedInit();
// Do background idle operation
void CONTROL_Idle();
// Update low-priority tasks
void CONTROL_UpdateLow();
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

void CONTROL_InitJSONPointers();

#endif // __CONTROLLER_H
