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
static Int16U ScopeDivCounter, ScopeDivErrCounter;
static Int16U ScopeDivCounterMax, ScopeDivErrCounterMax;
static Int16U ScopeValuesCounter, ErrorValuesCounter;
static MeasureCoeff Voltage, CurrentRange1, CurrentRange2, CurrentRange3;

// Forward functions
void MU_SaveSampleToEP(pDataSample Sample, Int16U Index);
void MU_InitCoeffX(pMeasureCoeff Coeff, Int16U StartRegister);
_iq MU_CalcX(pMeasureCoeff Coeff, Int32S RawValue, Boolean RMSFineCorrection);

// Functions
void MU_StartScope()
{
	ScopeDivCounterMax = ScopeDivErrCounterMax = DataTable[REG_SCOPE_RATE];
	ScopeDivCounter = ScopeDivErrCounter = 0;

	ScopeValuesCounter = MEMBUF_ScopeValues_Counter;
	ErrorValuesCounter = MEMBUF_ErrorValues_Counter;

	DL_PrepareLogging();
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_LogScopeValues, "ramfuncs");
#endif
void MU_LogScopeValues(pDataSampleIQ Instant, pDataSampleIQ RMS, Int16S PWM, Boolean SRAMDebug)
{
	static Int16U dbgVoltage = 0, dbgCurrent = 0;

	if(ScopeDivCounter++ >= ScopeDivCounterMax)
	{
		DataSample SampleToSave = {0};
		if(SRAMDebug)
		{
			SampleToSave.U.Data.Instant.Current = dbgCurrent;
			SampleToSave.U.Data.Instant.Voltage = dbgVoltage;

			if(dbgVoltage < 100)
				dbgVoltage++;
			else
				dbgVoltage = 0;
			dbgCurrent = dbgVoltage + 100;
		}
		else
		{
			SampleToSave.U.Data.Instant.Voltage = _IQint(Instant->Voltage);
			SampleToSave.U.Data.Instant.Current = _IQmpyI32int(Instant->Current, 1000);

			SampleToSave.U.Data.RMS.Voltage = _IQint(RMS->Voltage);
			SampleToSave.U.Data.RMS.Current = _IQmpyI32int(RMS->Current, 1000);

			SampleToSave.U.Data.PWM = PWM;
		}

		// Запись значений в SRAM
		DL_WriteData(&SampleToSave);

		// Запись значений в EP
		MU_SaveSampleToEP(&SampleToSave, ScopeValuesCounter);
		ScopeValuesCounter++;

		if(MEMBUF_ScopeValues_Counter < VALUES_x_SIZE)
			MEMBUF_ScopeValues_Counter = ScopeValuesCounter;

		if(ScopeValuesCounter >= VALUES_x_SIZE)
			ScopeValuesCounter = 0;

		ScopeDivCounter = 0;
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_SaveSampleToEP, "ramfuncs");
#endif
void MU_SaveSampleToEP(pDataSample Sample, Int16U Index)
{
	MEMBUF_Values_V[Index] = Sample->U.Data.Instant.Voltage;
	MEMBUF_Values_ImA[Index] = Sample->U.Data.Instant.Current / 1000;
	MEMBUF_Values_IuA[Index] = Sample->U.Data.Instant.Current % 1000;

	MEMBUF_Values_Vrms[Index] = Sample->U.Data.RMS.Voltage;
	MEMBUF_Values_Irms_mA[Index] = Sample->U.Data.RMS.Current / 1000;
	MEMBUF_Values_Irms_uA[Index] = Sample->U.Data.RMS.Current % 1000;

	MEMBUF_Values_PWM[Index] = Sample->U.Data.PWM;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_LogScopeError, "ramfuncs");
#endif
void MU_LogScopeError(Int16S Value)
{
	if(ScopeDivErrCounter++ >= ScopeDivErrCounterMax)
	{
		ScopeDivErrCounter = 0;
		MEMBUF_Values_Err[ErrorValuesCounter++] = Value;

		if(MEMBUF_ErrorValues_Counter < VALUES_x_SIZE)
			MEMBUF_ErrorValues_Counter = ErrorValuesCounter;

		if(ErrorValuesCounter >= VALUES_x_SIZE)
			ErrorValuesCounter = 0;
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
		{
			MU_SaveSampleToEP(&Sample, i);
			MEMBUF_ScopeValues_Counter = i;
		}
		else
			return;
	}
}
// ----------------------------------------

void MU_SeekScopeBack(Int16S Offset)
{
	DL_MoveReadPointer(-Offset);
}
// ----------------------------------------

void MU_InitCoeffX(pMeasureCoeff Coeff, Int16U StartRegister)
{
	Coeff->K =  _FPtoIQ2(DataTable[StartRegister], 1000);
	Coeff->P2 = _IQI((Int16S)DataTable[StartRegister + 1]);
	Coeff->P1 = _FPtoIQ2(DataTable[StartRegister + 2], 1000);
	Coeff->P0 = _FPtoIQ2(DataTable[StartRegister + 3], 10);
}
// ----------------------------------------

void MU_InitCoeffVoltage()
{
	MU_InitCoeffX(&Voltage, REG_COEFF_VOLTAGE_K);
}
// ----------------------------------------

void MU_InitCoeffCurrent1()
{
	MU_InitCoeffX(&CurrentRange1, REG_COEFF_CURRENT1_K);
}
// ----------------------------------------

void MU_InitCoeffCurrent2()
{
	MU_InitCoeffX(&CurrentRange2, REG_COEFF_CURRENT2_K);
}
// ----------------------------------------

void MU_InitCoeffCurrent3()
{
	MU_InitCoeffX(&CurrentRange3, REG_COEFF_CURRENT3_K);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_CalcX, "ramfuncs");
#endif
_iq MU_CalcX(pMeasureCoeff Coeff, Int32S RawValue, Boolean RMSFineCorrection)
{
	_iq tmp = _IQmpyI32(Coeff->K, RawValue);

	if(RMSFineCorrection)
	{
		_iq tmp2 = _IQdiv(tmp, _IQ(1000));
		return _IQmpy(tmp2, _IQmpy(tmp2, Coeff->P2)) + _IQmpy(tmp, Coeff->P1) + Coeff->P0;
	}
	else
		return tmp;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_CalcVoltage, "ramfuncs");
#endif
_iq MU_CalcVoltage(Int32S RawValue, Boolean RMSFineCorrection)
{
	return MU_CalcX(&Voltage, RawValue, RMSFineCorrection);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_CalcCurrent1, "ramfuncs");
#endif
_iq MU_CalcCurrent1(Int32S RawValue, Boolean RMSFineCorrection)
{
	return MU_CalcX(&CurrentRange1, RawValue, RMSFineCorrection);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_CalcCurrent2, "ramfuncs");
#endif
_iq MU_CalcCurrent2(Int32S RawValue, Boolean RMSFineCorrection)
{
	return MU_CalcX(&CurrentRange2, RawValue, RMSFineCorrection);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_CalcCurrent2, "ramfuncs");
#endif
_iq MU_CalcCurrent3(Int32S RawValue, Boolean RMSFineCorrection)
{
	return MU_CalcX(&CurrentRange3, RawValue, RMSFineCorrection);
}
// ----------------------------------------
