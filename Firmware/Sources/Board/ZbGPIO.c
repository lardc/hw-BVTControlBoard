// -----------------------------------------
// Board-specific GPIO functions
// ----------------------------------------

// Header
#include "ZbGPIO.h"


// Functions
//
void ZbGPIO_Init()
{
	// Reset to default state
	ZwGPIO_WritePin(PIN_LED_1, FALSE);
   	ZwGPIO_WritePin(PIN_POWER_EN1, FALSE);
   	ZwGPIO_WritePin(PIN_POWER_EN2, FALSE);
   	ZwGPIO_WritePin(PIN_POWER_EN3, FALSE);
   	ZwGPIO_WritePin(PIN_EXT_IND, FALSE);
   	ZwGPIO_WritePin(PIN_DEBUG, FALSE);
   	// Configure pins
   	ZwGPIO_PinToOutput(PIN_LED_1);
   	ZwGPIO_PinToOutput(PIN_POWER_EN1);
   	ZwGPIO_PinToOutput(PIN_POWER_EN2);
   	ZwGPIO_PinToOutput(PIN_POWER_EN3);
   	ZwGPIO_PinToOutput(PIN_EXT_IND);
   	ZwGPIO_PinToOutput(PIN_DEBUG);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(ZbGPIO_SwitchLED1, "ramfuncs");
#endif
void ZbGPIO_SwitchLED1(Boolean Set)
{
	ZwGPIO_WritePin(PIN_LED_1, Set);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(ZbGPIO_ToggleLED1, "ramfuncs");
#endif
void ZbGPIO_ToggleLED1()
{
	ZwGPIO_TogglePin(PIN_LED_1);
}
// ----------------------------------------

void ZbGPIO_SwitchPower(Boolean Enabled1, Boolean Enabled2)
{
   	ZwGPIO_WritePin(PIN_POWER_EN1, Enabled1);
   	ZwGPIO_WritePin(PIN_POWER_EN2, Enabled2);
}
// ----------------------------------------

void ZbGPIO_SwitchIndicator(Boolean Set)
{
   	ZwGPIO_WritePin(PIN_EXT_IND, Set);
}
// ----------------------------------------

// No more.
