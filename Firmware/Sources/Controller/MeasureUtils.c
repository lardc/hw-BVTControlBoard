// ----------------------------------------
// Utility functions for measurements
// ----------------------------------------

// Header
#include "MeasureUtils.h"

// Includes
#include "DeviceObjectDictionary.h"
#include "DataTable.h"
#include "IQmathUtils.h"
#include "Controller.h"

// Variables
static Int16U ScopeCounter, ScopeVICounter, ScopePWMCounter, ScopeErrCounter;
static Int16U ScopeCounterMax, ScopeVICounterMax, ScopePWMCounterMax, ScopeErrCounterMax;
static Int16U ValuesVI_Counter, ValuesPWM_Counter;

// Forward functions
void MU_LogScopeRaw(Int16S Voltage, Int32S Current);

// Functions
void MU_StartScope()
{
	ScopeCounterMax = ScopePWMCounterMax = ScopeVICounterMax = ScopeErrCounterMax = DataTable[REG_SCOPE_RATE];
	ScopeCounter = ScopeVICounter = ScopePWMCounter = ScopeErrCounter = 0;

	ValuesVI_Counter = MEMBUF_ValuesVI_Counter;
	ValuesPWM_Counter = MEMBUF_ValuesPWM_Counter;

	DL_PrepareLogging();
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_LogScopeVI, "ramfuncs");
#endif
void MU_LogScopeVI(pDataSampleIQ Sample, Boolean SRAMDebug)
{
	static Int16U dbgVoltage = 0, dbgCurrent = 0;

	if(ScopeCounter++ >= ScopeCounterMax)
	{
		DataSample SampleToSave;
		if(SRAMDebug)
		{
			SampleToSave.ScopeFields.Data.Current = dbgCurrent;
			SampleToSave.ScopeFields.Data.Voltage = dbgVoltage;

			if(dbgVoltage < 100)
				dbgVoltage++;
			else
				dbgVoltage = 0;
			dbgCurrent = dbgVoltage + 100;
		}
		else
		{
			SampleToSave.ScopeFields.Data.Voltage = _IQint(Sample->Voltage);
			SampleToSave.ScopeFields.Data.Current = _IQint(Sample->Current);
		}

		// Запись значений в SRAM
		DL_WriteData(&SampleToSave);

		// Запись значений в EP
		MEMBUF_Values_V[ValuesVI_Counter] = SampleToSave.ScopeFields.Data.Voltage;
		MEMBUF_Values_ImA[ValuesVI_Counter] = SampleToSave.ScopeFields.Data.Current / 1000;
		MEMBUF_Values_IuA[ValuesVI_Counter] = SampleToSave.ScopeFields.Data.Current % 1000;
		ValuesVI_Counter++;

		if(MEMBUF_ValuesVI_Counter < VALUES_x_SIZE)
			MEMBUF_ValuesVI_Counter = ValuesVI_Counter;

		if(ValuesVI_Counter >= VALUES_x_SIZE)
			ValuesVI_Counter = 0;

		ScopeCounter = 0;
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_LogScopeRaw, "ramfuncs");
#endif
void MU_LogScopeRaw(Int16S Voltage, Int32S Current)
{
	MEMBUF_Values_V[ValuesVI_Counter] = Voltage;
	MEMBUF_Values_ImA[ValuesVI_Counter] = Current / 1000;
	MEMBUF_Values_IuA[ValuesVI_Counter] = Current % 1000;
	MEMBUF_ValuesVI_Counter++;

	if(MEMBUF_ValuesVI_Counter >= VALUES_x_SIZE)
		MEMBUF_ValuesVI_Counter = 0;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_LogScopePWM, "ramfuncs");
#endif
void MU_LogScopePWM(Int16S Value)
{
	if(ValuesPWM_Counter >= VALUES_x_SIZE)
		ValuesPWM_Counter = 0;

	if(ScopePWMCounter++ >= ScopePWMCounterMax)
	{
		MEMBUF_Values_PWM[ValuesPWM_Counter++] = Value;
		ScopePWMCounter = 0;
	}

	if (MEMBUF_ValuesPWM_Counter < VALUES_x_SIZE)
		MEMBUF_ValuesPWM_Counter = ValuesPWM_Counter;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_LogScopeError, "ramfuncs");
#endif
void MU_LogScopeError(Int16S Value)
{
	if(MEMBUF_ValuesErr_Counter >= VALUES_x_SIZE)
		MEMBUF_ValuesErr_Counter = 0;

	if(ScopeErrCounter++ == ScopeErrCounterMax)
	{
		MEMBUF_Values_Err[MEMBUF_ValuesErr_Counter++] = Value;
		ScopeErrCounter = 0;
	}
}
// ----------------------------------------

void MU_LoadDataFragment()
{
	Int16U i;
	DataSample Sample;

	for(i = 0; i < VALUES_x_SIZE; ++i)
	{
		if(DL_ReadData(&Sample))
			MU_LogScopeRaw(Sample.ScopeFields.Data.Voltage, Sample.ScopeFields.Data.Current);
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
