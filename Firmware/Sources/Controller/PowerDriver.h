// -----------------------------------------
// Power driver
// ----------------------------------------

#ifndef __POWER_DRIVER_H
#define __POWER_DRIVER_H

// Include
#include "stdinc.h"
#include "ZwDSP.h"
#include "Global.h"


// Functions
//
// Init driver
void DRIVER_Init();
void DRIVER_SwitchPower24V();
void DRIVER_SwitchPower50V();
void DRIVER_SwitchPower100V();
void DRIVER_SwitchPower150V();
void DRIVER_SwitchPowerOff();
void DRIVER_PowerDischarge(Boolean State);
Int16U DRIVER_SwitchToTargetVoltage(Int16U SecondaryVoltage, Int16U Power, Int16U CurrentPrimaryVoltage,
		Int16U TransformerRatio);
void DRIVER_ClearTZFault();
Boolean DRIVER_GetShortPinState();


// Inline functions
//
// Switch generation of TZ interrupts by line
void inline DRIVER_EnableTZandInt(Boolean EnableSCProtection)
{
	ZwPWM_EnableTZInterruptsGlobal(FALSE);
	ZwPWM_ConfigTZ1(EnableSCProtection, PQ_Sample6);
	ZwPWM_EnableTZInterrupts(FALSE, FALSE, EnableSCProtection, FALSE, FALSE, FALSE);
	//
	ZwPWM_EnableTZInterruptsGlobal(EnableSCProtection);
}
// ----------------------------------------

#endif // __POWER_DRIVER_H
