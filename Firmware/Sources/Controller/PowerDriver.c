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
#define TZ_MASK_CBC_BRIDGE		0
#define TZ_MASK_OST_BRIDGE		(DBG_USE_BRIDGE_SHORT ? BIT0 : 0)

// Functions
//
void DRIVER_Init()
{
	// Enable TZ
	ZwPWM_SetTZPullup(PFDisable, PFDontcare, PFDontcare, PFDontcare, PFDontcare, PFDontcare);
	ZwPWM_ConfigTZ1(TRUE, PQ_Sample6);

	// Post TZ init delay
	DELAY_US(1000);

	// Init PWM outputs
	ZwPWMB_InitBridgeA12(CPU_FRQ, PWM_FREQUENCY, TZ_MASK_CBC_BRIDGE, TZ_MASK_OST_BRIDGE, PWM_SATURATION);
	ZwPWM3_Init(PWMUp, CPU_FRQ, PWM_FREQUENCY, FALSE, FALSE, TZ_MASK_CBC_BRIDGE, TZ_MASK_OST_BRIDGE, TRUE, TRUE, TRUE, FALSE, FALSE);

	// Clear possible faults
	DRIVER_ClearTZFault();

	// Configure TZ interrupts
	ZwPWM_ConfigTZIntOST(FALSE, FALSE, DBG_USE_BRIDGE_SHORT, FALSE, FALSE, FALSE);
}
// ----------------------------------------

void DRIVER_ClearTZFault()
{
	ZwPWM1_ClearTZ();
	ZwPWM2_ClearTZ();
	ZwPWM3_ClearTZ();
}
// ----------------------------------------

void DRIVER_SwitchPowerHigh()
{
	// Отключение разряда
	ZwGPIO_WritePin(PIN_DIS, TRUE);

	// Включение силовых БП
   	ZwGPIO_WritePin(PIN_POWER_1, TRUE);
   	ZwGPIO_WritePin(PIN_POWER_2, TRUE);
	ZwGPIO_WritePin(PIN_POWER_3, TRUE);
	ZwGPIO_WritePin(PIN_POWER_4, TRUE);
}
// ----------------------------------------

void DRIVER_SwitchPowerLow()
{
	// Отключение разряда
	ZwGPIO_WritePin(PIN_DIS, FALSE);

	// Включение низковольтного силового БП
   	ZwGPIO_WritePin(PIN_POWER_1, TRUE);
   	ZwGPIO_WritePin(PIN_POWER_2, FALSE);
	ZwGPIO_WritePin(PIN_POWER_3, FALSE);
	ZwGPIO_WritePin(PIN_POWER_4, FALSE);
}
// ----------------------------------------

void DRIVER_SwitchPowerOff()
{
	// Включение разряда
	ZwGPIO_WritePin(PIN_DIS, FALSE);

	// Отключение силовых БП
   	ZwGPIO_WritePin(PIN_POWER_1, FALSE);
   	ZwGPIO_WritePin(PIN_POWER_2, FALSE);
	ZwGPIO_WritePin(PIN_POWER_3, FALSE);
	ZwGPIO_WritePin(PIN_POWER_4, FALSE);
}
// ----------------------------------------

Boolean DRIVER_GetSHPinState()
{
	return ZwGPIO_ReadPin(PIN_SHORT);
}
// ----------------------------------------

// No more.
