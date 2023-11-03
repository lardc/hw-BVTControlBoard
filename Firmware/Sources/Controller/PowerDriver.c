// ----------------------------------------
// Power driver
// ----------------------------------------

// Header
#include "PowerDriver.h"
//
// Includes
#include "SysConfig.h"
#include "ZbBoard.h"
#include "Global.h"
#include "DeviceObjectDictionary.h"
#include "DataTable.h"

// Definitions
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
	Int16U TZMaskOstBridge = DataTable[REG_DISABLE_BRIDGE_SHORT] ? 0 : BIT0;
	ZwPWMB_InitBridge12(CPU_FRQ, PWM_FREQUENCY, TZ_MASK_CBC, TZMaskOstBridge, 0, PWM_SATURATION);
	ZwPWM3_Init(PWMUp, CPU_FRQ, PWM_FREQUENCY, FALSE, FALSE, TZ_MASK_CBC, TZMaskOstBridge, TRUE, TRUE, TRUE, FALSE, FALSE);

	// Clear possible faults
	DRIVER_ClearTZFault();

	// Configure TZ interrupts
	ZwPWM_ConfigTZIntOST(FALSE, FALSE, !DataTable[REG_DISABLE_BRIDGE_SHORT], FALSE, FALSE, FALSE);

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

Boolean DRIVER_GetSHPinState()
{
	return DataTable[REG_DISABLE_BRIDGE_SHORT] ? TRUE : ZwGPIO_ReadPin(PIN_SHORT);
}
// ----------------------------------------
