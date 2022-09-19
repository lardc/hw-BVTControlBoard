// ----------------------------------------
// Utility functions for measurements
// ----------------------------------------

#ifndef __MEASURE_UTIL_H
#define __MEASURE_UTIL_H

// Include
#include "stdinc.h"
#include "IQmathLib.h"
#include "DataLogger.h"

// Definitions
typedef struct __MeasureCoeff
{
	_iq K;
	_iq P2;
	_iq P1;
	_iq P0;
} MeasureCoeff, *pMeasureCoeff;

// Functions
// Init scope
void MU_StartScope();
// Log data to SRAM
void MU_LogScopeValues(pDataSampleIQ Instant, pDataSampleIQ RMS, Int16S PWM, Boolean SRAMDebug);
// Log following error
void MU_LogScopeError(_iq Value);
// Load data from SRAM
void MU_LoadDataFragment();
// Move scope pointer back
void MU_SeekScopeBack(Int16S Offset);

void MU_InitCoeffVoltage();
void MU_InitCoeffCurrent1();
void MU_InitCoeffCurrent2();
void MU_InitCoeffCurrent3();

_iq MU_CalcVoltage(Int32S RawValue, Boolean RMSFineCorrection);
_iq MU_CalcCurrent1(Int32S RawValue, Boolean RMSFineCorrection);
_iq MU_CalcCurrent2(Int32S RawValue, Boolean RMSFineCorrection);
_iq MU_CalcCurrent3(Int32S RawValue, Boolean RMSFineCorrection);

#endif // __MEASURE_UTIL_H
