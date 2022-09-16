// ----------------------------------------
// Log data to SRAM
// ----------------------------------------

// Header
#include "DataLogger.h"

// Includes
#include "ZbBoard.h"

// Variables
Int16U BankCounterW, BankCounterR;
Int32U AddressCounterW, DirectCounterW;
Int32U AddressCounterR, DirectCounterR;

// Functions
void DL_PrepareLogging()
{
	DirectCounterW = DirectCounterR = 0;
	AddressCounterW = AddressCounterR = 0;
	BankCounterW = BankCounterR = 0;
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(DL_WriteData, "ramfuncs");
#endif
void DL_WriteData(pDataSample Sample)
{
	ZbMemory_WriteValuesSRAM(BankCounterW, AddressCounterW, Sample->ScopeFields.Raw, sizeof(Sample->ScopeFields.Raw));

	++DirectCounterW;
	AddressCounterW += sizeof(Sample->ScopeFields.Raw);

	if(AddressCounterW >= SRAM_BANK_CAPACITY_SAMPLE)
	{
		AddressCounterW = 0;
		BankCounterW++;

		if(BankCounterW >= SRAM_BANK_COUNT)
		{
			BankCounterW = 0;
			DirectCounterW = 0;
		}
	}
}
// ----------------------------------------

Boolean DL_ReadData(pDataSample pData)
{
	DataSample buffer;

	if(DirectCounterR >= DirectCounterW)
		return FALSE;

	if(AddressCounterR >= SRAM_BANK_CAPACITY_SAMPLE)
	{
		AddressCounterR = 0;
		BankCounterR++;
	}

	ZbMemory_ReadValuesSRAM(BankCounterR, AddressCounterR, buffer.ScopeFields.Raw, sizeof(buffer.ScopeFields.Raw));

	++DirectCounterR;
	AddressCounterR += sizeof(buffer.ScopeFields.Raw);

	*pData = buffer;

	return TRUE;
}
// ----------------------------------------

void DL_MoveReadPointer(Int16S Offset)
{
	if(Offset >= 0)
	{
		DirectCounterR += Offset;

		if(DirectCounterR > DirectCounterW)
			DirectCounterR = DirectCounterW;

		AddressCounterR = (DirectCounterR * RAW_FIELDS_COUNT) % SRAM_BANK_CAPACITY_SAMPLE;
		BankCounterR = (DirectCounterR * RAW_FIELDS_COUNT) / SRAM_BANK_CAPACITY_SAMPLE;
	}
	else
	{
		Offset = -Offset;

		if(DirectCounterR < Offset)
			DirectCounterR += DirectCounterW;
		DirectCounterR -= Offset;

		AddressCounterR = (DirectCounterR * RAW_FIELDS_COUNT) % SRAM_BANK_CAPACITY_SAMPLE;
		BankCounterR = (DirectCounterR * RAW_FIELDS_COUNT) / SRAM_BANK_CAPACITY_SAMPLE;
	}
}
// ----------------------------------------
