// -----------------------------------------
// Board-specific GPIO functions
// ----------------------------------------

// Header
#include "ZbGPIO.h"
#include "ZwUtils.h"


// Functions
//
void ZbGPIO_Init()
{
	// Reset to default state
 	ZwGPIO_PinToOutput(PIN_LED_1);
 	ZwGPIO_PinToOutput(PIN_POWER_1);
 	ZwGPIO_PinToOutput(PIN_POWER_2);
 	ZwGPIO_PinToOutput(PIN_POWER_3);
 	ZwGPIO_PinToOutput(PIN_POWER_4);
	ZwGPIO_PinToOutput(PIN_EXT_IND);
	ZwGPIO_PinToOutput(PIN_FAN);
	ZwGPIO_PinToOutput(PIN_DIS);
	ZwGPIO_PinToOutput(PIN_SYNC);
	ZwGPIO_PinToOutput(PIN_SHORT_CLR);
	ZwGPIO_PinToOutput(PIN_PWM_1);
	ZwGPIO_PinToOutput(PIN_PWM_2);

	ZwGPIO_PinToOutput(PIN_MEM_A);
	ZwGPIO_PinToOutput(PIN_MEM_B);
	ZwGPIO_PinToOutput(PIN_MEM_C);


   	// Configure pins
   	ZwGPIO_WritePin(PIN_LED_1, FALSE);
   	ZwGPIO_WritePin(PIN_POWER_1, FALSE);
   	ZwGPIO_WritePin(PIN_POWER_2, FALSE);
	ZwGPIO_WritePin(PIN_POWER_3, FALSE);
	ZwGPIO_WritePin(PIN_POWER_4, FALSE);
	ZwGPIO_WritePin(PIN_EXT_IND, FALSE);
	ZwGPIO_WritePin(PIN_FAN, FALSE);
	ZwGPIO_WritePin(PIN_DIS, FALSE);
	ZwGPIO_WritePin(PIN_SYNC, FALSE);
	ZwGPIO_WritePin(PIN_SHORT_CLR, FALSE);
	ZwGPIO_WritePin(PIN_PWM_1, FALSE);
	ZwGPIO_WritePin(PIN_PWM_2, FALSE);
	ZwGPIO_WritePin(PIN_MEM_A, FALSE);
	ZwGPIO_WritePin(PIN_MEM_B, FALSE);
	ZwGPIO_WritePin(PIN_MEM_C, FALSE);
}
// ----------------------------------------

void ZbGPIO_SwitchFan(Boolean Set)
{
	ZwGPIO_WritePin(PIN_FAN, Set);
}
// ----------------------------------------

void ZbGPIO_SwitchLED1(Boolean Set)
{
	ZwGPIO_WritePin(PIN_LED_1, Set);
}
// ----------------------------------------

void ZbGPIO_ToggleLED1()
{
	ZwGPIO_TogglePin(PIN_LED_1);
}
// ----------------------------------------

void ZbGPIO_SwitchIndicator(Boolean Set)
{
   	ZwGPIO_WritePin(PIN_EXT_IND, Set);
}
// ----------------------------------------

void ZbGPIO_SwitchSYNC(Boolean Set)
{
	ZwGPIO_WritePin(PIN_SYNC, Set);
}
// ----------------------------------------

void ZbGPIO_ResetShortCircuit(Boolean Set)
{
	ZwGPIO_WritePin(PIN_SHORT_CLR, Set);
}
// ----------------------------------------
