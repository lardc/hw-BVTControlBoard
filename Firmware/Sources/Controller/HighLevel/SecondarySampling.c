// ----------------------------------------
// Communication with secondary side
// ----------------------------------------

// Header
#include "SecondarySampling.h"
//
// Includes
#include "ZwDSP.h"
#include "SysConfig.h"
#include "DeviceObjectDictionary.h"
#include "InterboardProtocol.h"
#include "Global.h"
#include "Controller.h"
#include "DataTable.h"
#include "IQmathUtils.h"


// Variables
//
_iq SS_Voltage = 0, SS_Current = 0;
Boolean SS_DataValid = FALSE;
//
Int16U InputBuffer[IBP_PACKET_SIZE];
//
static volatile Boolean SamplingActive = FALSE;


// Functions
//
void SS_ConfigureSensingCircuits(_iq CurrentSet, _iq VoltageSet)
{
	Int16U tmp, cmdBuffer[IBP_PACKET_SIZE] = { 0 };

	// Voltage configuration
	cmdBuffer[0] = (IBP_PACKET_START_BYTE << 8) | IBP_CMD_SET_ADC;

	// Определение границы верхнего диапазона тока
	_iq IHigh = DataTable[REG_OVERRIDE_MAX_CURRENT] ? _IQI(DataTable[REG_OVERRIDE_MAX_CURRENT] + HVD_I_ADD) : HVD_IH_TH;

	if (CurrentSet <= HVD_IL_TH)
		tmp = CurrentInput_Low;
	else if (CurrentSet <= IHigh)
		tmp = CurrentInput_High;
	else
		tmp = CurrentInput_High2;
	cmdBuffer[1] |= tmp;

	_iq LowLimit = DataTable[REG_CUSTOM_VLOW_LIMIT] ? _IQI(DataTable[REG_CUSTOM_VLOW_LIMIT]) : HVD_VL_TH;
	if (VoltageSet < LowLimit)
		tmp = VoltageInput_Low;
	else
		tmp = VoltageInput_High;
	cmdBuffer[1] |= tmp << 8;

	IBP_SendData(cmdBuffer, TRUE);
}
// ----------------------------------------

void SS_StartSampling()
{
	Int16U cmdBuffer[IBP_PACKET_SIZE] = { (IBP_PACKET_START_BYTE << 8) | IBP_CMD_SAMPLING, TRUE, 0, 0 };

	IBP_SendData(cmdBuffer, TRUE);
	SamplingActive = TRUE;
}
// ----------------------------------------

void SS_StopSampling()
{
	Int16U cmdBuffer[IBP_PACKET_SIZE] = { (IBP_PACKET_START_BYTE << 8) | IBP_CMD_SAMPLING, FALSE, 0, 0 };

	IBP_SendData(cmdBuffer, TRUE);
	SamplingActive = FALSE;
}
// ----------------------------------------

void SS_Dummy(Boolean UseTimeout)
{
	Int16U cmdBuffer[IBP_PACKET_SIZE] = { (IBP_PACKET_START_BYTE << 8) | IBP_CMD_DUMMY, 0, 0, 0 };

	IBP_SendData(cmdBuffer, UseTimeout);
}
// ----------------------------------------

Boolean SS_Ping()
{
	SS_DataValid = FALSE;
	SamplingActive = TRUE;

	SS_Dummy(FALSE);
	DELAY_US(1000);
	SS_DoSampling();
	DELAY_US(1000);

	SamplingActive = FALSE;
	return SS_DataValid;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(SS_HandleSlaveTransmission, "ramfuncs");
#endif
void SS_HandleSlaveTransmission()
{
	if(ZwSPIa_GetWordsToReceive() >= IBP_PACKET_SIZE)
	{
		ZwSPIa_EndReceive(InputBuffer, IBP_PACKET_SIZE);

		// Check header
		if (((InputBuffer[0] >> 8) == IBP_PACKET_START_BYTE) &&
			((InputBuffer[0] & 0x00FF) == IBP_ACK || (InputBuffer[0] & 0x00FF) == IBP_GET_DATA))
		{
			if ((InputBuffer[0] & 0x00FF) == IBP_ACK)
				IBP_SubcribeToTimeoutCycle(NULL);

			if (SamplingActive)
			{
				// | Current (24 bit)			|	Voltage (24 bit)		|
				// ||		16		||		8	|	8		||		16		||
				SS_Current = ((Int32U)InputBuffer[1]) << 16;
				SS_Current |= ((Int16U)InputBuffer[2]) & 0xFF00;
				SS_Voltage = ((Int32U)InputBuffer[2]) << 24;
				SS_Voltage |= ((Int32U)InputBuffer[3]) << 8;

				SS_DataValid = TRUE;

				MEASURE_AC_DoSampling();
			}
		}
		else
		{
			// Receive all waste clock pulses and re-init SPI
			DELAY_US(1);
			CONTROL_ReInitSPI_Rx();
			DataTable[REG_SPI_RX_RESETS]++;
		}
	}
}
// ----------------------------------------


