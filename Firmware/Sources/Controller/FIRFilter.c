// ----------------------------------------
// Implements FIR filter
// ----------------------------------------

// Header
#include "FIRFilter.h"
//
// Includes
#include "IQmathUtils.h"
#include "ZwUtils.h"

// Definitions
//
#define FIR_FILTER_LENGTH		31

// Constants
//
// 20kHz - sampling frequency
// 1kHz  - cutoff frequency
const _iq FIR_CalcTable[FIR_FILTER_LENGTH] = {
	_IQ(0.0003f),
	_IQ(0.0009f),
	_IQ(0.0015f),
	_IQ(0.0015f),
	_IQ(-0.0003f),
	_IQ(-0.0045f),
	_IQ(-0.0107f),
	_IQ(-0.0165f),
	_IQ(-0.0176f),
	_IQ(-0.0088f),
	_IQ(0.0136f),
	_IQ(0.0494f),
	_IQ(0.0937f),
	_IQ(0.1373f),
	_IQ(0.1693f),
	_IQ(0.1811f),
	_IQ(0.1693f),
	_IQ(0.1373f),
	_IQ(0.0937f),
	_IQ(0.0494f),
	_IQ(0.0136f),
	_IQ(-0.0088f),
	_IQ(-0.0176f),
	_IQ(-0.0165f),
	_IQ(-0.0107f),
	_IQ(-0.0045f),
	_IQ(-0.0003f),
	_IQ(0.0015f),
	_IQ(0.0015f),
	_IQ(0.0009f),
	_IQ(0.0003f)
};

// Variables
//
static volatile _iq FIR_BufferV[FIR_FILTER_LENGTH], FIR_BufferI[FIR_FILTER_LENGTH];
static volatile Int16U FIR_BufferCounter = 0;

// Functions
//
void FIR_Reset()
{
	Int16U i;

	FIR_BufferCounter = 0;
	for(i = 0; i < FIR_FILTER_LENGTH; i++)
		FIR_BufferV[i] = FIR_BufferI[i] = 0;
}

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(FIR_LoadValues, "ramfuncs");
#endif
void FIR_LoadValues(_iq V, _iq I)
{
	Int16U i;

	if (FIR_BufferCounter < FIR_FILTER_LENGTH)
	{
		FIR_BufferV[FIR_BufferCounter] = V;
		FIR_BufferI[FIR_BufferCounter] = I;
		FIR_BufferCounter++;
	}
	else
	{
		for (i = 1; i < FIR_FILTER_LENGTH; i++)
		{
			FIR_BufferV[i - 1] = FIR_BufferV[i];
			FIR_BufferI[i - 1] = FIR_BufferI[i];
		}

		FIR_BufferV[FIR_FILTER_LENGTH - 1] = V;
		FIR_BufferI[FIR_FILTER_LENGTH - 1] = I;
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(FIR_Apply, "ramfuncs");
#endif
void FIR_Apply(_iq *V, _iq *I)
{
	Int16U i;
	_iq Vres = 0, Ires = 0;

	for (i = 0; i < FIR_FILTER_LENGTH; i++)
	{
		Vres += _IQmpy(FIR_CalcTable[i], FIR_BufferV[i]);
		Ires += _IQmpy(FIR_CalcTable[i], FIR_BufferI[i]);
	}

	*V = Vres;
	*I = Ires;
}
// ----------------------------------------

// No more
