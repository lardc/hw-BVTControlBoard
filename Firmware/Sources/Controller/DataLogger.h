// ----------------------------------------
// Log data to SRAM
// ----------------------------------------

#ifndef __DATA_LOGGER_H
#define __DATA_LOGGER_H

// Include
#include "stdinc.h"
#include "IQmathLib.h"

// Definitions
#define RAW_FIELDS_COUNT	7

// Types
typedef struct __DataSample
{
	union
	{
		Int16U Raw[RAW_FIELDS_COUNT];
		struct
		{
			Int16S Voltage;
			Int16S VoltageRMS;
			Int16S PWM;
			Int16U Current;
			Int16U CurrentRMS;
			Int16U CurrentTails;
			Int16S CosPhi;
		} Data;
	} U;
} DataSample, *pDataSample;

typedef struct __DataSampleIQ
{
	_iq Voltage;
	_iq Current;
} DataSampleIQ, *pDataSampleIQ;

// Functions
// Initialize logger for a new work
void DL_PrepareLogging();
// Write data sample to SRAM
void DL_WriteData(pDataSample Sample);
// Read data back from SRAM
Boolean DL_ReadData(pDataSample pData);
// Move read pointer with specified offset
void DL_MoveReadPointer(Int16S Offset);

#endif // __DATA_LOGGER_H
