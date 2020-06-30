// -----------------------------------------
// Logic of self-test process
// ----------------------------------------

// Header
#include "SelfTest.h"
//
// Includes
#include "SysConfig.h"
#include "ZbBoard.h"
#include "DeviceObjectDictionary.h"
#include "Global.h"
#include "DataTable.h"
#include "PowerDriver.h"
#include "Controller.h"


// Functions
//
Boolean ST_ValidateConnections(pInt16U pFault)
{

	if(DBG_USE_BRIDGE_SHORT && !DRIVER_GetSHPinState())
	{
		*pFault = DISABLE_NO_SHORT_SIGNAL;
		return FALSE;
	}

	return TRUE;
}
// ----------------------------------------

// No more.
