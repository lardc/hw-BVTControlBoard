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

typedef void (*PSFunction)();
typedef struct __MWPowerSettings
{
	Int16U Voltage;
	Int16U Power;
	PSFunction Function;
} MWPowerSettings;

MWPowerSettings MWPowerSettingsArray[POWER_OPTIONS_MAXNUM] = {
		{24,	150,	DRIVER_SwitchPower24V},
		{50,	500,	DRIVER_SwitchPower50V},
		{100,	1000,	DRIVER_SwitchPower100V},
		{200,	1500,	DRIVER_SwitchPower200V}
};

// Functions
//
void DRIVER_Init()
{
	// Init PWM outputs
	ZwPWMB_InitBridgeA12(CPU_FRQ, PWM_FREQUENCY, 0, 0, PWM_SATURATION);
}
// ----------------------------------------

void DRIVER_SwitchPower24V()
{
	ZwGPIO_WritePin(PIN_POWER_1, TRUE);
	ZwGPIO_WritePin(PIN_POWER_2, FALSE);
	ZwGPIO_WritePin(PIN_POWER_3, FALSE);
	ZwGPIO_WritePin(PIN_POWER_4, FALSE);
}
// ----------------------------------------

void DRIVER_SwitchPower50V()
{
	ZwGPIO_WritePin(PIN_POWER_1, FALSE);
	ZwGPIO_WritePin(PIN_POWER_2, TRUE);
	ZwGPIO_WritePin(PIN_POWER_3, FALSE);
	ZwGPIO_WritePin(PIN_POWER_4, FALSE);
}
// ----------------------------------------

void DRIVER_SwitchPower100V()
{
	ZwGPIO_WritePin(PIN_POWER_1, FALSE);
	ZwGPIO_WritePin(PIN_POWER_2, TRUE);
	ZwGPIO_WritePin(PIN_POWER_3, FALSE);
	ZwGPIO_WritePin(PIN_POWER_4, TRUE);
}
// ----------------------------------------

void DRIVER_SwitchPower200V()
{
	ZwGPIO_WritePin(PIN_POWER_1, FALSE);
	ZwGPIO_WritePin(PIN_POWER_2, TRUE);
	ZwGPIO_WritePin(PIN_POWER_3, TRUE);
	ZwGPIO_WritePin(PIN_POWER_4, TRUE);
}
// ----------------------------------------

void DRIVER_PowerDischarge(Boolean State)
{
	ZwGPIO_WritePin(PIN_DIS, !State);
}
// ----------------------------------------

void DRIVER_SwitchPowerOff()
{
	DRIVER_PowerDischarge(TRUE);
	
	ZwGPIO_WritePin(PIN_POWER_1, FALSE);
	ZwGPIO_WritePin(PIN_POWER_2, FALSE);
	ZwGPIO_WritePin(PIN_POWER_3, FALSE);
	ZwGPIO_WritePin(PIN_POWER_4, FALSE);
}
// ----------------------------------------

Int16U DRIVER_SwitchToTargetVoltage(Int16U SecondaryVoltage, Int16U Power, Int16U CurrentPrimaryVoltage,
		Int16U TransformerRatio, Int16U PowerOptionsCount)
{
	Int16U i, PrimaryVoltage;

	Int16U TargetPrimaryVoltage = (Int32U)SecondaryVoltage * (100 + CAP_POW_VOLT_MARGIN) / 100 / TransformerRatio;
	Int16U TargetPower = (Int32U)Power * (100 + CAP_POW_VOLT_MARGIN) / 100;
	
	if(PowerOptionsCount > POWER_OPTIONS_MAXNUM)
		PowerOptionsCount = POWER_OPTIONS_MAXNUM;

	for(i = 0; i < PowerOptionsCount; i++)
	{
		if((TargetPrimaryVoltage < MWPowerSettingsArray[i].Voltage && TargetPower < MWPowerSettingsArray[i].Power)
				|| i == (PowerOptionsCount - 1))
		{
			PrimaryVoltage = MWPowerSettingsArray[i].Voltage;
			MWPowerSettingsArray[i].Function();

			if(CurrentPrimaryVoltage >= (Int32U)PrimaryVoltage * (100 + CAP_VOLTAGE_DELTA) / 100)
				DRIVER_PowerDischarge(TRUE);
			break;
		}
	}

	return PrimaryVoltage;
}
// ----------------------------------------

Boolean DRIVER_IsShortCircuit()
{
	return !ZwGPIO_ReadPin(PIN_SHORT);
}
// ----------------------------------------
