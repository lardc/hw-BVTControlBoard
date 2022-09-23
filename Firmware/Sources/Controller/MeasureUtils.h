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
	Int16U Kn;
	Int32S Kd;
	_iq P2;
	_iq P1;
	_iq P0;
} MeasureCoeff, *pMeasureCoeff;

// Functions
// Init scope
void MU_StartScope();
// Log data to SRAM
void MU_LogScopeValues(pDataSampleIQ Instant, pDataSampleIQ RMS, _iq CosPhi, Int16S PWM, Boolean SRAMDebug);
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

_iq MU_CalcVoltage(_iq RawValue, Boolean RMSFineCorrection);
_iq MU_CalcCurrent1(_iq RawValue, Boolean RMSFineCorrection);
_iq MU_CalcCurrent2(_iq RawValue, Boolean RMSFineCorrection);
_iq MU_CalcCurrent3(_iq RawValue, Boolean RMSFineCorrection);

#endif // __MEASURE_UTIL_H
