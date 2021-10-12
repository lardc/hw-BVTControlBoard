// -----------------------------------------
// Power driver
// ----------------------------------------

// Header
#include "PowerDriver.h"
//
// Includes
#include "SysConfig.h"
#include "ZbBoard.h"
#include "Global.h"

// Definitions
#define TZ_MASK_OST_BRIDGE		(DBG_USE_BRIDGE_SHORT ? BIT0 : 0)
#define TZ_MASK_CBC				0

// Functions
//
void DRIVER_Init()
{
	// Enable TZ
	ZwPWM_SetTZPullup(PFDisable, PFDontcare, PFDisable, PFDontcare, PFDontcare, PFDontcare);
	ZwPWM_ConfigTZ1(TRUE, PQ_Sample6);
	ZwPWM_ConfigTZ3(TRUE, PQ_Sample6);

	// Post TZ init delay
	DELAY_US(1000);

	// Init PWM outputs
	ZwPWMB_InitBridge12(CPU_FRQ, PWM_FREQUENCY, TZ_MASK_CBC, TZ_MASK_OST_BRIDGE, 0, PWM_SATURATION);
	ZwPWM3_Init(PWMUp, CPU_FRQ, PWM_FREQUENCY, FALSE, FALSE, TZ_MASK_CBC, TZ_MASK_OST_BRIDGE, TRUE, TRUE, TRUE, FALSE, FALSE);

	// Clear possible faults
	DRIVER_ClearTZFault();

	// Configure TZ interrupts
	ZwPWM_ConfigTZIntOST(FALSE, FALSE, DBG_USE_BRIDGE_SHORT, FALSE, FALSE, FALSE);

	// Configure temperature sensing pin
	ZwGPIO_PinToInput(PIN_TFAULT, FALSE, PQ_Sample6);
}
// ----------------------------------------

void DRIVER_ClearTZFault()
{
	ZwPWM1_ClearTZ();
	ZwPWM2_ClearTZ();
	ZwPWM3_ClearTZ();
	ZwPWM4_ClearTZ();
}
// ----------------------------------------

void DRIVER_SwitchPower(Boolean Enable1, Boolean Enable2)
{
   if (!Enable1 && !Enable2)
      ZwGPIO_WritePin(PIN_POWER_EN3, FALSE);
   else
      ZwGPIO_WritePin(PIN_POWER_EN3, TRUE);

   ZbGPIO_SwitchPower(Enable1, Enable2);
}
// ----------------------------------------

Boolean DRIVER_ReadTemperatureFault()
{
	static Int16U FilterCounter = 0;

	if (!ZwGPIO_ReadPin(PIN_TFAULT))
	{
		if (FilterCounter < 100)
			FilterCounter++;
		else
			return TRUE;
	}
	else
		FilterCounter = 0;

	return FALSE;
}
// ----------------------------------------

Boolean DRIVER_GetSHPinState()
{
	return ZwGPIO_ReadPin(PIN_SHORT);
}
// ----------------------------------------

// No more.
