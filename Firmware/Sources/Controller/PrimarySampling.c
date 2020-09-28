// -----------------------------------------
// Monitoring of capacitors voltage and primary winding current
// ----------------------------------------

// Header
#include "PrimarySampling.h"
//
// Includes
#include "ZwDSP.h"
#include "DataTable.h"
#include "DeviceObjectDictionary.h"
#include "IQmathUtils.h"

// Constants
//
#define SAMPLE_V_FILTER_LENGTH		32
#define SAMPLE_V_FILTER_2ORDER		5
//
static const Int16U ADCChannelVC[16] = { AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP,
										 AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP, AIN_V_CAP };

// Variables
//
static Int16U SamplesVCounter;
static _iq CapacitorVCoefficient, CapacitorVoltage = 0;
static Int16U SamplesV[SAMPLE_V_FILTER_LENGTH];
//
static volatile Boolean IPCalCompletedFlag = FALSE;

// Forward functions
//
static void PSAMPLING_MonitoringVCRoutine(Int16U * const restrict aSampleVector);

// Functions
//
void PSAMPLING_Init()
{
	CapacitorVCoefficient = _FPtoIQ2(DataTable[REG_CAP_V_COFF_N], DataTable[REG_CAP_V_COFF_D]);
}
// ----------------------------------------

void PSAMPLING_ConfigureSamplingVCap()
{
	ZwADC_ConfigureSequentialCascaded(16, ADCChannelVC);
	ZwADC_SubscribeToResults1(PSAMPLING_MonitoringVCRoutine);

	SamplesVCounter = 0;
	MemZero16(SamplesV, SAMPLE_V_FILTER_LENGTH);
}
// ----------------------------------------

void PSAMPLING_DoSamplingVCap()
{
	ZwADC_StartSEQ1();
}
// ----------------------------------------

Int16U PSAMPLING_ReadCapVoltage()
{
	return _IQint(CapacitorVoltage);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(PSAMPLING_MonitoringVCRoutine, "ramfuncs");
#endif
static void PSAMPLING_MonitoringVCRoutine(Int16U * const restrict aSampleVector)
{
	Int16U i, filteredV;
	Int32U sum = 0;

	// Accumulate ADC result
	#pragma UNROLL(16)
	for(i = 0; i < 16; ++i)
		sum += aSampleVector[i];

	// Do noise rejection via oversampling and circular buffer
	SamplesV[SamplesVCounter++] = (sum >> 4);
	if(SamplesVCounter == SAMPLE_V_FILTER_LENGTH)
		SamplesVCounter = 0;

	// Do IIR on V and I
	sum = 0;
	for(i = 0; i < SAMPLE_V_FILTER_LENGTH; ++i)
		sum += SamplesV[i];

	filteredV = sum >> SAMPLE_V_FILTER_2ORDER;

	// Convert to IQ values
	CapacitorVoltage = _IQmpyI32(CapacitorVCoefficient, filteredV);
}
// ----------------------------------------

// No more.
