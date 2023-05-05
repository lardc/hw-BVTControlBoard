// ----------------------------------------
// Driver for EEPROM & FRAM via SPI
// ----------------------------------------

// Header
#include "ZbMemory.h"
//
#include "SysConfig.h"


// Constants
//
#define DATA_BUFFER_SIZE		16
#define EPROM_DATA_SEGMENT		4		// 4 * 2 + (3) < 16 - SPI FIFO max depth
#define EPROM_WRITE_DELAY_US	5000
//
// Commands
#define MEM_WRITE				0x02
#define MEM_READ				0x03
#define EPROM_WREN				0x06
//
// SPI states
#define SPIMODE_EPROM			0
#define SPIMODE_SRAM			1


// Variables
//
static Int16U DataBuffer[DATA_BUFFER_SIZE];
static Int16U SPIMode;


// Forward functions
//
static void ZbMemory_EnableWriteEPROM();
static void ZbMemory_SelectBank(Int16U MemoryBank);
static void ZwMemory_PrepareSPIForSRAM();
static void ZwMemory_PrepareSPIForEPROM();


// Functions
//
void ZbMemory_Init()
{
	SPIMode = SPIMODE_EPROM;

	// Init SPI-C
	ZwSPIc_Init(TRUE, MEM_EPROM_BAUDRATE, 8, MEM_PLR, MEM_PHASE, ZW_SPI_INIT_TX | ZW_SPI_INIT_RX | ZW_SPI_INIT_CS, FALSE, FALSE);
	ZwSPIc_InitFIFO(0, 0);
	ZwSPIc_ConfigInterrupts(FALSE, FALSE);
	ZwSPIc_EnableInterrupts(FALSE, TRUE);

	// Initialize MUX pins
	ZwGPIO_WritePin(PIN_MEM_A, TRUE);
	ZwGPIO_WritePin(PIN_MEM_B, TRUE);
	ZwGPIO_WritePin(PIN_MEM_C, TRUE);
	ZwGPIO_PinToOutput(PIN_MEM_A);
	ZwGPIO_PinToOutput(PIN_MEM_B);
	ZwGPIO_PinToOutput(PIN_MEM_C);
}
// ----------------------------------------

void inline ZbMemory_SelectBank(Int16U MemoryBank)
{
	ZwGPIO_WritePin(PIN_MEM_A, MemoryBank & BIT0);
	ZwGPIO_WritePin(PIN_MEM_B, MemoryBank & BIT1);
	ZwGPIO_WritePin(PIN_MEM_C, MemoryBank & BIT2);
	DELAY_US(10);
}
// ----------------------------------------

void ZbMemory_WriteValuesEPROM(Int16U EPROMAddress, pInt16U Buffer, Int16U BufferSize)
{
	Int16U i, j, segCount;

	// Select EPROM on the bus
	ZbMemory_SelectBank(MEM_BANK_EPROM);
	ZwMemory_PrepareSPIForEPROM();
	
	// Calculate segment count: only 16 SRAM bytes can be written per one transaction (FIFO limit)
	segCount = (BufferSize / EPROM_DATA_SEGMENT) + ((BufferSize % EPROM_DATA_SEGMENT) ? 1 : 0); 
		
	// Write segments
	for(j = 0; j < segCount; ++j)
	{
		Int16U dataSize;
		
		// Calculate address for next segment
		Int16U currentEPROMAddress = EPROMAddress + j * EPROM_DATA_SEGMENT * 2;

		// Enable writing for next operation
		ZbMemory_EnableWriteEPROM();

		// Write command ID
		DataBuffer[0] = MEM_WRITE;
		// Memory address
		DataBuffer[1] = (currentEPROMAddress >> 8);
		DataBuffer[2] = currentEPROMAddress & 0x00FF;
		// Write data
		for(i = 0; i < MIN(BufferSize - j * EPROM_DATA_SEGMENT, EPROM_DATA_SEGMENT); ++i)
		{
			DataBuffer[3 + i * 2] = Buffer[j * EPROM_DATA_SEGMENT + i] >> 8;
			DataBuffer[3 + i * 2 + 1] = Buffer[j * EPROM_DATA_SEGMENT + i] & 0x00FF;
		}
	
		// Do SPI communication
		dataSize = 3 + MIN(BufferSize - j * EPROM_DATA_SEGMENT, EPROM_DATA_SEGMENT) * 2;
		DINT;
		ZwSPIc_Send(DataBuffer, dataSize, 8, STTNormal);
		EINT;

		DELAY_US(EPROM_WRITE_DELAY_US);
	}
}
// ----------------------------------------

void ZbMemory_ReadValuesEPROM(Int16U EPROMAddress, pInt16U Buffer, Int16U BufferSize)
{
	Int16U i, j, segCount, dataSize;

	// Select EPROM on the bus
	ZbMemory_SelectBank(MEM_BANK_EPROM);
	ZwMemory_PrepareSPIForEPROM();

	// Calculate segment count: only 16 FRAM bytes can be read per one transaction (FIFO limit)
	segCount = (BufferSize / EPROM_DATA_SEGMENT) + ((BufferSize % EPROM_DATA_SEGMENT) ? 1 : 0); 
		
	// Read segments
	for(j = 0; j < segCount; ++j)
	{
		// Calculate address for next segment
		Int16U currentEPROMAddress = EPROMAddress + j * EPROM_DATA_SEGMENT * 2;
		
		// Write command ID
		DataBuffer[0] = MEM_READ;
		// Memory address
		DataBuffer[1] = (currentEPROMAddress >> 8);
		DataBuffer[2] = currentEPROMAddress & 0x00FF;
		
		// Do SPI communication
		dataSize = 3 + MIN(BufferSize - j * EPROM_DATA_SEGMENT, EPROM_DATA_SEGMENT) * 2;
		DINT;
		ZwSPIc_BeginReceive(DataBuffer, dataSize, 8, STTNormal);
		EINT;
		while(ZwSPIc_GetWordsToReceive() < dataSize)
			DELAY_US(1);
		ZwSPIc_EndReceive(DataBuffer, dataSize);
		
		// Copy data
		for(i = 0; i < MIN(BufferSize - j * EPROM_DATA_SEGMENT, EPROM_DATA_SEGMENT); ++i)
		{
			Int16U result;
			
			// Get data from bytes
			result = (DataBuffer[3 + i * 2] & 0x00FF) << 8;
			result |= DataBuffer[3 + i * 2 + 1] & 0x00FF;
			
			Buffer[j * EPROM_DATA_SEGMENT + i] = result;
		}
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(ZbMemory_WriteValuesSRAM, "ramfuncs");
#endif
void ZbMemory_WriteValuesSRAM(Int16U Bank, Int32U SRAMAddress, pInt16U Buffer, Int16U BufferSize)
{
	static Int16U LastBank = INT16U_MAX;

	if(LastBank != Bank)
	{
		while(!GpioDataRegs.GPADAT.bit.SPI_C_CS)
			asm(" NOP");

		ZbMemory_SelectBank(Bank);
		LastBank = Bank;
	}

	ZwMemory_PrepareSPIForSRAM();
	// Write command ID
	DataBuffer[0] = (MEM_WRITE << 8) | ((SRAMAddress << 1) >> 16);
	// Memory address
	DataBuffer[1] = (SRAMAddress << 1) & 0xFFFF;

	DINT;
	ZwSPIc_Send(DataBuffer, 2, 16, STTNormal);
	ZwSPIc_Send(Buffer, BufferSize, 16, STTStream);
	EINT;
}
// ----------------------------------------

void ZbMemory_ReadValuesSRAM(Int16U Bank, Int32U SRAMAddress, pInt16U Buffer, Int16U BufferSize)
{
	// Select SRAM on the bus
	ZbMemory_SelectBank(Bank);
	ZwMemory_PrepareSPIForSRAM();

	// Write command ID
	DataBuffer[0] = (MEM_READ << 8) | ((SRAMAddress << 1) >> 16);
	// Memory address
	DataBuffer[1] = (SRAMAddress << 1) & 0xFFFF;
	DataBuffer[2] = 0;
	DataBuffer[3] = 0;

	// Do receive
	DINT;
	ZwSPIc_BeginReceive(DataBuffer, BufferSize + 2, 16, STTNormal);
	EINT;
	while(ZwSPIc_GetWordsToReceive() < (BufferSize + 2))
		DELAY_US(1);
	ZwSPIc_EndReceive(DataBuffer, 2);
	ZwSPIc_EndReceive(Buffer, BufferSize);
}
// ----------------------------------------

static void ZbMemory_EnableWriteEPROM()
{
	// Write @Enable@ command
	DataBuffer[0] = EPROM_WREN;
	// Do SPI communication
	ZwSPIc_Send(DataBuffer, 1, 8, STTNormal);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(ZwMemory_PrepareSPIForSRAM, "ramfuncs");
#endif
static void ZwMemory_PrepareSPIForSRAM()
{
	if (SPIMode != SPIMODE_SRAM)
	{
		// Config SPI for SRAM
		ZwSPIc_Init(TRUE, MEM_SRAM_BAUDRATE, 16, MEM_PLR, MEM_PHASE, 0, TRUE, FALSE);
		DELAY_US(1);
		SPIMode = SPIMODE_SRAM;
	}
}
// ----------------------------------------

static void ZwMemory_PrepareSPIForEPROM()
{
	if (SPIMode != SPIMODE_EPROM)
	{
		// Config SPI for EPROM
		ZwSPIc_Init(TRUE, MEM_EPROM_BAUDRATE, 8, MEM_PLR, MEM_PHASE, 0, TRUE, FALSE);
		DELAY_US(1);
		SPIMode = SPIMODE_EPROM;
	}
}
// ----------------------------------------


