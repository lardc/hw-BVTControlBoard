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
#define FIR_FILTER_LENGTH		7

// Constants
//
const _iq FIR_CalcTable[FIR_FILTER_LENGTH] = {
	_IQ(0.03),
	_IQ(0.11),
	_IQ(0.22),
	_IQ(0.28),
	_IQ(0.22),
	_IQ(0.11),
	_IQ(0.03)
};

// Variables
//
static volatile _iq FIR_BufferV[FIR_FILTER_LENGTH], FIR_BufferI[FIR_FILTER_LENGTH];
static volatile Int16U FIR_BufferCounter = 0;
static Boolean BufferFull = FALSE;

// Functions
//
void FIR_Reset()
{
	Int16U i;

	BufferFull = FALSE;
	FIR_BufferCounter = 0;
	for(i = 0; i < FIR_FILTER_LENGTH; i++)
		FIR_BufferV[i] = FIR_BufferI[i] = 0;
}

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(FIR_LoadValues, "ramfuncs");
#endif
void FIR_LoadValues(_iq V, _iq I)
{
	if(FIR_BufferCounter >= FIR_FILTER_LENGTH)
	{
		FIR_BufferCounter = 0;
		BufferFull = TRUE;
	}

	FIR_BufferV[FIR_BufferCounter] = V;
	FIR_BufferI[FIR_BufferCounter] = I;
	FIR_BufferCounter++;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(FIR_Apply, "ramfuncs");
#endif
void FIR_Apply(_iq *V, _iq *I)
{
	Int16U i, j;
	_iq Vres = 0, Ires = 0;

	if(BufferFull)
	{
		// Обработка более старых значений
		for(i = FIR_BufferCounter, j = 0; i < FIR_FILTER_LENGTH; i++, j++)
		{
			Vres += _IQmpy(FIR_CalcTable[j], FIR_BufferV[i]);
			Ires += _IQmpy(FIR_CalcTable[j], FIR_BufferI[i]);
		}

		// Обработка более новых значений
		for(i = 0, j = FIR_FILTER_LENGTH - FIR_BufferCounter; i < FIR_BufferCounter; i++, j++)
		{
			Vres += _IQmpy(FIR_CalcTable[j], FIR_BufferV[i]);
			Ires += _IQmpy(FIR_CalcTable[j], FIR_BufferI[i]);
		}
	}
	else
	{
		for(i = 0; i < FIR_FILTER_LENGTH; i++)
		{
			Vres += _IQmpy(FIR_CalcTable[i], FIR_BufferV[i]);
			Ires += _IQmpy(FIR_CalcTable[i], FIR_BufferI[i]);
		}
	}

	*V = Vres;
	*I = Ires;
}
// ----------------------------------------


