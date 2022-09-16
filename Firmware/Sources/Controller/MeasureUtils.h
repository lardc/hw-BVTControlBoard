// ----------------------------------------
// Utility functions for measurements
// ----------------------------------------

#ifndef __MEASURE_UTIL_H
#define __MEASURE_UTIL_H

// Include
#include "stdinc.h"
#include "IQmathLib.h"
#include "DataLogger.h"

// Functions
// Init scope
void MU_StartScope();
// Log data to SRAM
void MU_LogScopeVI(pDataSampleIQ Sample, Boolean SRAMDebug);
// Log diagnostic values
void MU_LogScopePWM(Int16S Value);
// Log following error
void MU_LogScopeError(Int16S Value);
// Load data from SRAM
void MU_LoadDataFragment();
// Move scope pointer back
void MU_SeekScopeBack(Int16S Offset);

#endif // __MEASURE_UTIL_H
