// ----------------------------------------
// Utility functions for measurements
// ----------------------------------------

// Header
#include "MeasureUtils.h"
//
// Includes
#include "DeviceObjectDictionary.h"
#include "DataTable.h"
#include "MemoryBuffers.h"

#include "IQmathUtils.h"


// Variables
//
static Int16U ScopeCounter, ScopeIVCounter, ScopeDiagCounter;
static Int16U ScopeCounterMax, ScopeIVCounterMax, ScopeDiagCounterMax;
static Int16U ValuesIV_Counter, ValuesDIAG_Counter;

// Functions
//
void MU_StartScope()
{
	ScopeCounterMax = ScopeDiagCounterMax = ScopeIVCounterMax = DataTable[REG_SCOPE_RATE];
	ScopeCounter = ScopeIVCounter = ScopeDiagCounter = 0;

	ValuesIV_Counter = MEMBUF_ValuesIV_Counter;
	ValuesDIAG_Counter = MEMBUF_ValuesDIAG_Counter;

	DL_PrepareLogging();
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_LogScope, "ramfuncs");
#endif
void MU_LogScope(pDataSample Sample, Int16U CurrentMultiply, Boolean SRAMDebug, Boolean UseDualPolarity)
{
	static Int16U dbgVoltage = 0, dbgCurrent = 0;
	Int32S voltage, current;

	if(ScopeCounter++ >= ScopeCounterMax)
	{
		if (!SRAMDebug)
		{
			current = _IQmpyI32int(Sample->IQFields.Current, CurrentMultiply);
			voltage = _IQint(Sample->IQFields.Voltage);

#ifdef SCOPE_DATA_INVERT
			Sample->ScopeFields.Data.Current = (current < 0 && !UseDualPolarity) ? 0 : -current;
			Sample->ScopeFields.Data.Voltage = (voltage < 0 && !UseDualPolarity) ? 0 : -voltage;
#else
			Sample->ScopeFields.Data.Current = (current > 0 && !UseDualPolarity) ? 0 : current;
			Sample->ScopeFields.Data.Voltage = (voltage > 0 && !UseDualPolarity) ? 0 : voltage;
#endif
		}
		else
		{
			Sample->ScopeFields.Data.Current = dbgCurrent;
			Sample->ScopeFields.Data.Voltage = dbgVoltage;

			if (dbgVoltage < 100)
				dbgVoltage++;
			else
				dbgVoltage = 0;
			dbgCurrent = dbgVoltage + 100;
		}

		DL_WriteData(Sample);
		ScopeCounter = 0;
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_LogScopeIV, "ramfuncs");
#endif
Boolean MU_LogScopeIV(DataSample ActualSample)
{
	if(ValuesIV_Counter >= VALUES_xBIG_SIZE)
		ValuesIV_Counter = 0;

	if(ScopeIVCounter++ >= ScopeIVCounterMax)
	{
		MEMBUF_Values_I[ValuesIV_Counter] = _IQmpyI32int(ActualSample.IQFields.Current, DIAG_CURRENT_MUL);
		MEMBUF_Values_V[ValuesIV_Counter] = _IQint(ActualSample.IQFields.Voltage);
		ValuesIV_Counter++;

		ScopeIVCounter = 0;
	}

	if (MEMBUF_ValuesIV_Counter < VALUES_xBIG_SIZE)
		MEMBUF_ValuesIV_Counter = ValuesIV_Counter;

	return TRUE;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_LogScopeIVpeak, "ramfuncs");
#endif
void MU_LogScopeIVpeak(DataSampleIQ ActualSample)
{
	if(MEMBUF_ValuesIVpeak_Counter >= VALUES_x_SIZE)
		MEMBUF_ValuesIVpeak_Counter = 0;

	MEMBUF_Values_Ipeak[MEMBUF_ValuesIVpeak_Counter] = (ActualSample.Current > 0) ? _IQmpyI32int(ActualSample.Current, 100) : 0;
	MEMBUF_Values_Vpeak[MEMBUF_ValuesIVpeak_Counter] = _IQint(ActualSample.Voltage);
	++MEMBUF_ValuesIVpeak_Counter;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_LogScopeRaw, "ramfuncs");
#endif
Boolean MU_LogScopeRaw(Int16S V1, Int16S V2, Boolean IgnoreRate)
{
	if(MEMBUF_ValuesIV_Counter >= VALUES_xBIG_SIZE)
		MEMBUF_ValuesIV_Counter = 0;

	if(IgnoreRate || (ScopeIVCounter++ >= ScopeIVCounterMax))
	{
		MEMBUF_Values_I[MEMBUF_ValuesIV_Counter] = V1;
		MEMBUF_Values_V[MEMBUF_ValuesIV_Counter] = V2;
		MEMBUF_ValuesIV_Counter++;

		ScopeIVCounter = 0;
	}

	return TRUE;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_LogScopeDIAG, "ramfuncs");
#endif
Boolean MU_LogScopeDIAG(Int16S Value)
{
	if(ValuesDIAG_Counter >= VALUES_xBIG_SIZE)
		ValuesDIAG_Counter = 0;

	if(ScopeDiagCounter++ >= ScopeDiagCounterMax)
	{
		MEMBUF_Values_DIAG[ValuesDIAG_Counter++] = Value;
		ScopeDiagCounter = 0;
	}

	if (MEMBUF_ValuesDIAG_Counter < VALUES_xBIG_SIZE)
		MEMBUF_ValuesDIAG_Counter = ValuesDIAG_Counter;

	return TRUE;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_LogScopeErr, "ramfuncs");
#endif
void MU_LogScopeErr(Int16S Value)
{
	if(MEMBUF_ValuesErr_Counter >= VALUES_x_SIZE)
		MEMBUF_ValuesErr_Counter = 0;

	MEMBUF_Values_Err[MEMBUF_ValuesErr_Counter++] = Value;
}
// ----------------------------------------

void MU_LoadDataFragment()
{
	Int16U i;
	DataSample Sample;

	for(i = 0; i < VALUES_xBIG_SIZE; ++i)
	{
		if(DL_ReadData(&Sample))
			MU_LogScopeRaw(Sample.ScopeFields.Data.Voltage, Sample.ScopeFields.Data.Current, TRUE);
		else
			break;
	}
}
// ----------------------------------------

void MU_SeekScopeBack(Int16S Offset)
{
	DL_MoveReadPointer(-Offset);
}
// ----------------------------------------

void MU_ReplaceIVbyPeakData()
{
	Int16U i;

	MEMBUF_ValuesIV_Counter = MEMBUF_ValuesIVpeak_Counter;
	for(i = 0; i < MEMBUF_ValuesIV_Counter; ++i)
	{
		// I and V swapped
		MEMBUF_Values_V[i] = -((Int16S)(MEMBUF_Values_Ipeak[i] / 10));
		MEMBUF_Values_I[i] = -((Int16S)MEMBUF_Values_Vpeak[i]);
	}
}
// ----------------------------------------


