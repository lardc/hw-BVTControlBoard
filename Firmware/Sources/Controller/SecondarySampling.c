// ----------------------------------------
// Communication with secondary side
// ----------------------------------------

// Header
#include "SecondarySampling.h"

// Includes
#include "ZwDSP.h"
#include "DeviceObjectDictionary.h"
#include "DataTable.h"
#include "Controller.h"
#include "SysConfig.h"

// Definitions
#define IBP_GET_DATA				1
#define IBP_CMD_CFG_SWITCH			2
#define IBP_CMD_PING				3

#define WAIT_DELAY					(3 * 1000000L / (SPIB_BAUDRATE / IBP_CHAR_SIZE / IBP_PACKET_SIZE))

// Variables
static Boolean RxAck;
Int16U SS_Voltage, SS_Current;
static Int16U InputBuffer[IBP_PACKET_SIZE];

// Forward function
Boolean SS_SendX(Int16U Header, Int16U Data, Boolean WaitAck);

// Functions
#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(SS_SendX, "ramfuncs");
#endif
Boolean SS_SendX(Int16U Header, Int16U Data, Boolean WaitAck)
{
	static Int16U Buffer[IBP_PACKET_SIZE];

	Buffer[0] = (IBP_PACKET_START_BYTE << 8) | Header;
	Buffer[1] = Data;
	if(WaitAck)
		RxAck = FALSE;
	ZwSPIb_BeginReceive(Buffer, IBP_PACKET_SIZE, IBP_CHAR_SIZE, STTStream);

	if(WaitAck)
		DELAY_US(WAIT_DELAY);

	return RxAck;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(SS_GetData, "ramfuncs");
#endif
Boolean SS_GetData(Boolean WaitAck)
{
	return SS_SendX(IBP_GET_DATA, 0, WaitAck);
}
// ----------------------------------------

Boolean SS_SelectShunt(SwitchConfig Config)
{
	return SS_SendX(IBP_CMD_CFG_SWITCH, Config, TRUE);
}
// ----------------------------------------

Boolean SS_Ping()
{
	return SS_SendX(IBP_CMD_PING, 0, TRUE);
}
// ----------------------------------------

void SS_GetLastMessage()
{
	Int16U i;
	for(i = 0; i < IBP_PACKET_SIZE; i++)
		DataTable[REG_DIAG_DIGI_PACKET_B1 + i] = InputBuffer[i];
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
		if((InputBuffer[0] >> 8) == IBP_PACKET_START_BYTE)
		{
			switch(InputBuffer[0] & 0x00FF)
			{
				case IBP_GET_DATA:
					RxAck = TRUE;
					SS_Voltage = InputBuffer[1];
					SS_Current = InputBuffer[2];
					CONTROL_DataReceiveAck = TRUE;
					CONTROL_DataPostReceiveRoutine();
					break;

				case IBP_CMD_CFG_SWITCH:
				case IBP_CMD_PING:
					RxAck = TRUE;
					break;

				default:
					break;
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
