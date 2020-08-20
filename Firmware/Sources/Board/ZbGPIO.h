// -----------------------------------------
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
void ZbGPIO_SwitchFan(Boolean Set);
// Switch LED 1
void ZbGPIO_SwitchLED1(Boolean Set);
// Toggle LED 1
void ZbGPIO_ToggleLED1();
// Switch external indicator
void ZbGPIO_SwitchIndicator(Boolean Set);
// Switch SYNC
void ZbGPIO_SwitchSYNC(Boolean Set);
void ZbGPIO_ResetShortCircuit();


#endif // __ZBGPIO_H
