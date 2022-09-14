// ----------------------------------------
// Communication with secondary side
// ----------------------------------------

#ifndef __SECONDARY_SAMPLING_H
#define __SECONDARY_SAMPLING_H

// Include
#include "stdinc.h"

// Enums
typedef enum __SwitchConfig
{
	SwitchConfig_I1 = 0,
	SwitchConfig_I2,
	SwitchConfig_I3
} SwitchConfig;

// Definitions
// Protocol
#define IBP_PACKET_START_BYTE		0xA6u
#define IBP_HEADER_SIZE				1
#define IBP_BODY_SIZE				2
#define IBP_PACKET_SIZE				(IBP_HEADER_SIZE + IBP_BODY_SIZE)
#define IBP_CHAR_SIZE				16

// Variables
extern Int16U SS_Voltage, SS_Current;

// Functions
Boolean SS_GetData(Boolean WaitAck);
Boolean SS_SelectShunt(SwitchConfig Config);
Boolean SS_Ping();
void SS_GetLastMessage();
void SS_HandleSlaveTransmission();

#endif // __SECONDARY_SAMPLING_H
