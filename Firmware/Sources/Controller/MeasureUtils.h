// -----------------------------------------
// Utility functions for measurements
// ----------------------------------------

#ifndef __MEASURE_UTIL_H
#define __MEASURE_UTIL_H

// Include
#include "stdinc.h"
//
#include "IQmathLib.h"
#include "DataLogger.h"


// Functions
//
// Init scope
void MU_StartScope();
// Log data to SRAM
void MU_LogScope(pDataSample Sample, Int16U CurrentMultiply, Boolean SRAMDebug, Boolean UseDualPolarity);
// Log data to scope sequences
Boolean MU_LogScopeIV(DataSample ActualSample);
// Log peak data to scope sequences
void MU_LogScopeIVpeak(DataSampleIQ ActualSample);
// Log data to scope sequences in raw format
Boolean MU_LogScopeRaw(Int16S V1, Int16S V2, Boolean IgnoreRate);
// Log diagnostic values
Boolean MU_LogScopeDIAG(Int16S Value);
// Log following error
Boolean MU_LogScopeErr(Int16S Value);
// Load data from SRAM
void MU_LoadDataFragment();
// Move scope pointer back
void MU_SeekScopeBack(Int16S Offset);
// Replace I/V curves data by I/V peak data
void MU_ReplaceIVbyPeakData();


#endif // __MEASURE_UTIL_H
