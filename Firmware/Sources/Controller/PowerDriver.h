#ifndef __POWER_DRIVER_H
#define __POWER_DRIVER_H

// Include
#include "stdinc.h"
#include "ZwDSP.h"
#include "Global.h"

// Definitions
#define POWER_OPTIONS_MAXNUM		4

// Functions
void DRIVER_Init();
void DRIVER_SwitchPower24V();
void DRIVER_SwitchPower50V();
void DRIVER_SwitchPower100V();
void DRIVER_SwitchPower150V();
void DRIVER_SwitchPowerOff();
void DRIVER_PowerDischarge(Boolean State);
Int16U DRIVER_SwitchToTargetVoltage(Int16U SecondaryVoltage, Int16U Power, Int16U CurrentPrimaryVoltage,
		Int16U TransformerRatio, Int16U PowerOptionsCount);
Boolean DRIVER_IsShortCircuit();

#endif // __POWER_DRIVER_H
