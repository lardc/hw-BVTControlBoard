// Header
#include "InterboardProtocol.h"
//
// Includes
#include "SysConfig.h"
#include "ZbBoard.h"
#include "Global.h"
#include "Controller.h"

// Forward functions
//
void IBP_TimeoutFunction();

// Variables
//
static volatile Int16U IBP_Timeout = 0;
static volatile IBP_FUNC_HighSpeedTimeoutRoutine HighSpeedTimeoutRoutine = NULL;

// Functions
//
#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(IBP_SendData, "ramfuncs");
#endif
void IBP_SendData(pInt16U DataBuffer, Boolean UseTimeout)
{
	// Timeout for non-realtime opeations
	if (UseTimeout && DBG_USE_OPTO_TIMEOUT)
	{
		EINT;
		while (HighSpeedTimeoutRoutine);

		// Init timeout
		if ((DataBuffer[0] & 0xFF) != IBP_CMD_DUMMY)
		{
			IBP_Timeout = IBP_TIMEOUT;
			IBP_SubcribeToTimeoutCycle(IBP_TimeoutFunction);
		}
	}

	// Send data to slave
	ZwSPIb_BeginReceive(DataBuffer, IBP_PACKET_SIZE, 16, STTStream);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(IBP_TimeoutFunction, "ramfuncs");
#endif
void IBP_TimeoutFunction()
{
	if (IBP_Timeout > 0)
		IBP_Timeout--;
	else
	{
		IBP_SubcribeToTimeoutCycle(NULL);
		CONTROL_RequestStop(DF_OPTO_CON_ERROR, FALSE);
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(IBP_HighSpeedTimeoutCycle, "ramfuncs");
#endif
void IBP_HighSpeedTimeoutCycle()
{
	if (HighSpeedTimeoutRoutine)
		HighSpeedTimeoutRoutine();
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(IBP_SubcribeToTimeoutCycle, "ramfuncs");
#endif
void IBP_SubcribeToTimeoutCycle(IBP_FUNC_HighSpeedTimeoutRoutine Routine)
{
	HighSpeedTimeoutRoutine = Routine;
}
// ----------------------------------------

// No more.
