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
_iq MU_CalcX(pMeasureCoeff Coeff, _iq RawValue, Boolean RMSFineCorrection);

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
			SampleToSave.U.Data.Voltage = dbgVoltage;
			SampleToSave.U.Data.Current = dbgCurrent;

			if(dbgCurrent < 100)
				dbgCurrent++;
			else
				dbgCurrent = 0;
			dbgVoltage = dbgCurrent + 100;
		}
		else
		{
			SampleToSave.U.Data.Voltage = _IQint(Instant->Voltage);
			SampleToSave.U.Data.VoltageRMS = _IQint(RMS->Voltage);
			SampleToSave.U.Data.PWM = PWM;

			// Упаковка значений тока из расчёта 24бит на каждое значение
			Int32U Current = (Int32U)_IQmpyI32int(Instant->Current, 1000);
			SampleToSave.U.Data.Current = (Current >> 8) & 0xffff;
			SampleToSave.U.Data.CurrentTails = Current & 0xff;

			Int32U CurrentRMS = (Int32U)_IQmpyI32int(RMS->Current, 1000);
			SampleToSave.U.Data.CurrentRMS = (CurrentRMS >> 8) & 0xffff;
			SampleToSave.U.Data.CurrentTails |= (CurrentRMS & 0xff) << 8;
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
	MEMBUF_Values_V[Index] = Sample->U.Data.Voltage;
	MEMBUF_Values_Vrms[Index] = Sample->U.Data.VoltageRMS;
	MEMBUF_Values_PWM[Index] = Sample->U.Data.PWM;

	// Распаковка значений тока
	// Мгновенное
	Int32U Current = Sample->U.Data.Current;
	Current = (Current << 8) | (Sample->U.Data.CurrentTails & 0xff);
	if(Current & BIT23)
		Current |= 0xff000000;
	Int32S sCurrent = (Int32S)Current;

	MEMBUF_Values_ImA[Index] = sCurrent / 1000;
	MEMBUF_Values_IuA[Index] = sCurrent % 1000;

	// Действующее
	Current = Sample->U.Data.CurrentRMS;
	Current = (Current << 8) | (Sample->U.Data.CurrentTails >> 8);
	if(Current & BIT23)
		Current |= 0xff000000;
	sCurrent = (Int32S)Current;

	MEMBUF_Values_Irms_mA[Index] = sCurrent / 1000;
	MEMBUF_Values_Irms_uA[Index] = sCurrent % 1000;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_LogScopeError, "ramfuncs");
#endif
void MU_LogScopeError(_iq Value)
{
	if(ScopeDivErrCounter++ >= ScopeDivErrCounterMax)
	{
		ScopeDivErrCounter = 0;
		MEMBUF_Values_Err[ErrorValuesCounter++] = _IQint(Value);

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
	Coeff->Kn = DataTable[StartRegister];
	Coeff->Kd = 10l * DataTable[StartRegister + 1];
	Coeff->P2 = _IQI((Int16S)DataTable[StartRegister + 2]);
	Coeff->P1 = _FPtoIQ2(DataTable[StartRegister + 3], 1000);
	Coeff->P0 = _FPtoIQ2(DataTable[StartRegister + 4], 10);
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
_iq MU_CalcX(pMeasureCoeff Coeff, _iq RawValue, Boolean RMSFineCorrection)
{
	Int32S RawK = _IQ3mpyI32int(_IQtoIQ3(RawValue), Coeff->Kn);
	_iq tmp = _IQI(RawK / Coeff->Kd) + _FPtoIQ2(RawK % Coeff->Kd, Coeff->Kd);

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
_iq MU_CalcVoltage(_iq RawValue, Boolean RMSFineCorrection)
{
	return MU_CalcX(&Voltage, RawValue, RMSFineCorrection);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_CalcCurrent1, "ramfuncs");
#endif
_iq MU_CalcCurrent1(_iq RawValue, Boolean RMSFineCorrection)
{
	return MU_CalcX(&CurrentRange1, RawValue, RMSFineCorrection);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_CalcCurrent2, "ramfuncs");
#endif
_iq MU_CalcCurrent2(_iq RawValue, Boolean RMSFineCorrection)
{
	return MU_CalcX(&CurrentRange2, RawValue, RMSFineCorrection);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(MU_CalcCurrent2, "ramfuncs");
#endif
_iq MU_CalcCurrent3(_iq RawValue, Boolean RMSFineCorrection)
{
	return MU_CalcX(&CurrentRange3, RawValue, RMSFineCorrection);
}
// ----------------------------------------
