// -----------------------------------------
// Device exchange protocol
// ----------------------------------------

#ifndef __INTER_BOARD_PROTOCOL_H
#define __INTER_BOARD_PROTOCOL_H

// Include
#include "stdinc.h"

// Variables
//
extern volatile Boolean WaitForACK;
//
typedef void (*IBP_FUNC_HighSpeedTimeoutRoutine)();

// Constants
//
// Protocol
#define IBP_PACKET_START_BYTE		0xA6u
#define IBP_HEADER_SIZE				1
#define IBP_BODY_SIZE				3
#define IBP_PACKET_SIZE				(IBP_HEADER_SIZE + IBP_BODY_SIZE)
#define IBP_CHAR_SIZE 				16
#define IBP_TIMEOUT					20	// in TIMER0 ticks
//
// Commands
#define IBP_GET_DATA				0
#define IBP_CMD_SAMPLING			1
// 2-4
#define IBP_CMD_CFG_SWITCH			5
#define IBP_CMD_SET_PWM				6
#define IBP_CMD_SET_ADC				7
#define IBP_CMD_DUMMY				0xFE
#define IBP_ACK						0xFF

// Enums
//
// Current sensing chain
typedef enum __CurrentInputs
{
	CurrentInput_Low = 0,
	CurrentInput_High,
	CurrentInput_DC_Low,
	CurrentInput_DC_High
} CurrentInputs;

// Voltage sensing chain
typedef enum __VoltageInputs
{
	VoltageInput_Low = 0,
	VoltageInput_High
} VoltageInputs;

// Switch configuration
typedef enum __SwitchConfig
{
	SwitchConfig_BV = 0,
	SwitchConfig_AC,
	SwitchConfig_DC
} SwitchConfig;

// Functions
void IBP_SendData(pInt16U DataBuffer, Boolean UseTimeout);
void IBP_SubcribeToTimeoutCycle(IBP_FUNC_HighSpeedTimeoutRoutine Routine);
void IBP_HighSpeedTimeoutCycle();

#endif // __INTER_BOARD_PROTOCOL_H
