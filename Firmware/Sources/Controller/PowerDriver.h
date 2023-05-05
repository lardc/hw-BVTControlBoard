// ----------------------------------------
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
// Connect/disconnect capacitors battery
void DRIVER_SwitchPower(Boolean Enable1, Boolean Enable2);
// Clear TZ condition
void DRIVER_ClearTZFault();
// Check temperature lines state
Boolean DRIVER_ReadTemperatureFault();
// Get TZ pin state for bridge short circuit
Boolean DRIVER_GetSHPinState();


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
