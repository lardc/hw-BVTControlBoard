// ----------------------------------------
// Board-specific GPIO functions
// ----------------------------------------

#ifndef __ZBGPIO_H
#define __ZBGPIO_H

// Include
#include "stdinc.h"
#include "ZwDSP.h"

// Functions
//
// Initialize board GPIO
void ZbGPIO_Init();
// Switch LED 1
void ZbGPIO_SwitchLED1(Boolean Set);
// Toggle LED 1
void ZbGPIO_ToggleLED1();
// Switch external power
void ZbGPIO_SwitchPowerInvert(Boolean Flag);
void ZbGPIO_SwitchPower(Boolean Enabled1, Boolean Enabled2);
// Switch external indicator
void ZbGPIO_SwitchIndicator(Boolean Set);
void ZbGPIO_SwitchSYNC(Boolean Set);

#endif // __ZBGPIO_H
