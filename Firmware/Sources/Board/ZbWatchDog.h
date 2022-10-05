// -----------------------------------------
// Driver for watch-dog circuit
// ----------------------------------------

#ifndef __ZBWD_H
#define __ZBWD_H

// Include
#include "stdinc.h"
#include "ZwDSP.h"

// Functions
//
// Init watch-dog
void ZbWatchDog_Init();

// Inline functions
//
// Clear watch-dog timer
void inline ZbWatchDog_Strobe()
{
	ZwGPIO_TogglePin(PIN_WD_RST);
}

#endif // __ZBWD_H
