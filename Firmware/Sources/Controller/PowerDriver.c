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
#define POWER_OPTIONS_MAX		3
#define CAP_POW_VOLT_MARGIN		20		// Запас по мощности и напряжению в процентах

typedef struct __MWPowerSettings
{
	Int16U Voltage;
	Int16U Power;
	PSFunction Function;
} MWPowerSettings;

// Forward power functions
void DRIVER_SwitchPower12V();
void DRIVER_SwitchPower50V();
void DRIVER_SwitchPower150V();

static MWPowerSettings MWPowerSettingsArray[POWER_OPTIONS_MAX] = {
		{12,	25,	DRIVER_SwitchPower12V},
		{50,	250,	DRIVER_SwitchPower50V},
		{150,	1500,	DRIVER_SwitchPower150V}};
PSFunction PrimaryPSOperationFunc = NULL;

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

void DRIVER_SwitchPower12V()
{
	DRIVER_SwitchPower(FALSE, FALSE);
}
// ----------------------------------------

void DRIVER_SwitchPower50V()
{
	DRIVER_SwitchPower(TRUE, FALSE);
}
// ----------------------------------------

void DRIVER_SwitchPower150V()
{
	DRIVER_SwitchPower(TRUE, TRUE);
}
// ----------------------------------------

Int16U DRIVER_SwitchToTargetVoltage(Int16U ActualPrimaryVoltage)
{
	Int32U SecondaryVoltage = DataTable[REG_LIMIT_VOLTAGE];
	Int32U OutputPower = SecondaryVoltage * DataTable[REG_LIMIT_CURRENT] / 10000;

	// Средняя мощность в одополупериодном импульсном режиме составляет 0,25 от максимальной
	// и рассчитывается по формуле
	// integrate sin(2 * pi * nu * x)^2 dx, x=0..(1 / (2 * nu)), где nu - частота импульсов
	// OutputPower /= 4;

	Int16U TargetPrimaryVoltage = SecondaryVoltage * (100 + CAP_POW_VOLT_MARGIN) / 100 / DataTable[REG_TRANSFORMER_COFF];
	Int16U TargetPower = OutputPower * (100 + CAP_POW_VOLT_MARGIN) / 100;

	Int16U i;
	Int32U PrimaryVoltage = 0;
	for(i = 0; i < POWER_OPTIONS_MAX; i++)
	{
		PrimaryVoltage = MWPowerSettingsArray[i].Voltage;
		if((TargetPrimaryVoltage < PrimaryVoltage && TargetPower < MWPowerSettingsArray[i].Power)
				|| i == (POWER_OPTIONS_MAX - 1))
		{
			PrimaryPSOperationFunc = MWPowerSettingsArray[i].Function;

			if(ActualPrimaryVoltage > PrimaryVoltage * (100 + CAP_VOLTAGE_DELTA) / 100)
				DRIVER_SwitchPower(FALSE, FALSE);
			else
				MWPowerSettingsArray[i].Function();
			break;
		}
	}

	return PrimaryVoltage;
}
// ----------------------------------------
