// -----------------------------------------
// Driver for watch-dog circuit
// ----------------------------------------

// Header
#include "ZbWatchDog.h"


// Functions
//
void ZbWatchDog_Init()
{
	// Init LDAC pins
	ZwGPIO_WritePin(PIN_WD_RST, FALSE);
	ZwGPIO_PinToOutput(PIN_WD_RST);
}

// No more.
