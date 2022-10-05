// -----------------------------------------
// Communication with secondary side
// ----------------------------------------

#ifndef __SECONDARY_SAMPLING_H
#define __SECONDARY_SAMPLING_H

// Include
#include "stdinc.h"
//
#include "ZwDSP.h"
#include "InterboardProtocol.h"
#include "Global.h"

// Variables
//
extern _iq SS_Current, SS_Voltage;
extern Boolean SS_DataValid;

// Functions
//
// Configure current and voltage sensing
void SS_ConfigureSensingCircuits(_iq CurrentSet, _iq VoltageSet, Boolean ModeDC);
// Perform proper commutation
void SS_Commutate(SwitchConfig State);
// PWM value for digitizer flyback DC transformer
void SS_SetPWM(Int16U Value);
// Start sampling
void SS_StartSampling();
// Stop sampling
void SS_StopSampling();
// Handle slave packet
void SS_HandleSlaveTransmission();
// Dummy command to finalize command set
void SS_Dummy(Boolean UseTimeout);
// Ping digitizer
Boolean SS_Ping();


// Inline function
//
void inline SS_DoSampling()
{
	Int16U SPIBuffer[IBP_PACKET_SIZE] = { (IBP_PACKET_START_BYTE << 8) | IBP_GET_DATA, 0, 0, 0 };

	// Send data to slave
	IBP_SendData(SPIBuffer, FALSE);
}
// ----------------------------------------

#endif // __SECONDARY_SAMPLING_H
