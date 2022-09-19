// ----------------------------------------
// Samples battery voltage
// ----------------------------------------

// Header
#include "PrimarySampling.h"

// Includes
#include "ZwDSP.h"
#include "DataTable.h"
#include "DeviceObjectDictionary.h"

// Constants
#define SAMPLE_LENGTH					16
static const Int16U ADCChannelVC[SAMPLE_LENGTH] = {AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP,
		AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP};

// Functions
void PS_Init()
{
	ZwADC_ConfigureSequentialCascaded(16, ADCChannelVC);
}
// ----------------------------------------

Int16U PS_GetBatteryVoltage()
{
	ZwADC_StartSEQ1();
	while(ZwADC_IsSEQ1Busy());

	Int32U i, Result = 0;
	pInt16U ResStartAddr = ZwADC_GetValues1();
	for(i = 0; i < SAMPLE_LENGTH; i++)
		Result += *(ResStartAddr + i);

	ZwADC_ProcessInterruptSEQ1();
	return Result * DataTable[REG_CAP_COEFF] / 1000 / SAMPLE_LENGTH;
}
// ----------------------------------------
