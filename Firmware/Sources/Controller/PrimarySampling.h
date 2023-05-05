// ----------------------------------------
// Monitoring of capacitors voltage and primary winding current
// ----------------------------------------

#ifndef __PRIMARY_SAMPLING_H
#define __PRIMARY_SAMPLING_H

// Include
#include "stdinc.h"
#include "IQmathLib.h"

// Variables
//
extern volatile _iq PSAMPLING_CapacitorVoltage;

// Functions
//
// Init monitor
void PSAMPLING_Init();
// Configure for capacitor voltage monitoring
void PSAMPLING_ConfigureSamplingVCap();
// Sample capacitor voltage
void PSAMPLING_DoSamplingVCap();

#endif // __PRIMARY_SAMPLING_H
