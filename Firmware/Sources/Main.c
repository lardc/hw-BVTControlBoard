﻿// ----------------------------------------
// Program entry point
// ----------------------------------------

// Include
#include <stdinc.h>
//
#include "ZwDSP.h"
#include "ZbBoard.h"
#include "SysConfig.h"
#include "Controller.h"
#include "SecondarySampling.h"


// FORWARD FUNCTIONS
// -----------------------------------------
Boolean InitializeCPU();
void InitializeTimers();
void InitializeADC();
void InitializeSPI();
void InitializeSCI();
void InitializeCAN();
void InitializePWM();
void InitializeBoard();
void InitializeController();
// -----------------------------------------

// FORWARD ISRs
// -----------------------------------------
// CPU Timer 0 ISR
ISRCALL Timer0_ISR();
// CPU Timer 2 ISR
ISRCALL Timer2_ISR();
// CAN Line 0 ISR
ISRCALL CAN0_ISR();
// ADC SEQ1 ISR
ISRCALL SEQ1_ISR();
// SPI-A RX ISR
ISRCALL SPIaRX_ISR();
// ILLEGAL ISR
ISRCALL IllegalInstruction_ISR();
// -----------------------------------------

// FUNCTIONS
// -----------------------------------------
// Program main function
void main()
{
	Int16U i;

	// Boot process
	InitializeCPU();

	// Switch GPIO in proper state
	InitializeBoard();

   	// Wait for power-on
   	for(i = 0; i < MSC_PON_DELAY_MS; ++i)
   	{
   		DELAY_US(1000);
   		ZbWatchDog_Strobe();
   	}

	// Only if good clocking was established
	InitializeTimers();
	InitializeADC();
	InitializeSPI();
	InitializeSCI();
	InitializeCAN();
	InitializePWM();

	// Setup ISRs
	BEGIN_ISR_MAP
		ADD_ISR(TINT0, Timer0_ISR);
		ADD_ISR(TINT2, Timer2_ISR);
		ADD_ISR(ECAN0INTA, CAN0_ISR);
		ADD_ISR(SEQ1INT, SEQ1_ISR);
		ADD_ISR(SPIRXINTA, SPIaRX_ISR);
		ADD_ISR(ILLEGAL, IllegalInstruction_ISR);
	END_ISR_MAP

	// Init board external watch-dog
   	ZbWatchDog_Init();

   	// Initialize controller logic
	InitializeController();

	// Enable interrupts
	EINT;
	ERTM;

	// Set watch-dog as WDRST
	ZwSystem_SelectDogFunc(FALSE);
	ZwSystem_EnableDog(SYS_WD_PRESCALER);
	ZwSystem_LockDog();

	// Start timers
	ZwTimer_StartT2();

	// Background cycle
	while(TRUE)
		CONTROL_Idle();
}
// -----------------------------------------

// Initialize and prepare DSP
Boolean InitializeCPU()
{
    Boolean clockInitResult;

	// Init clock and peripherals
    clockInitResult = ZwSystem_Init(CPU_PLL, CPU_CLKINDIV, SYS_LOSPCP, SYS_HISPCP, SYS_PUMOD);

    if(clockInitResult)
    {
		// Do default GPIO configuration
		ZwGPIO_Init(GPIO_TSAMPLE, GPIO_TSAMPLE, GPIO_TSAMPLE, GPIO_TSAMPLE, GPIO_TSAMPLE);
		// Initialize PIE
		ZwPIE_Init();
		// Prepare PIE vectors
		ZwPIE_Prepare();
    }

	// Configure flash
	ZW_FLASH_CODE_SHADOW;
	ZW_FLASH_MATH_SHADOW;
	ZW_FLASH_OPTIMIZE(FLASH_FWAIT, FLASH_OTPWAIT);

   	return clockInitResult;
}
// -----------------------------------------

// Initialize CPU timers
void InitializeTimers()
{
    ZwTimer_InitT0();
	ZwTimer_SetT0(TIMER0_PERIOD);
	ZwTimer_EnableInterruptsT0(TRUE);

    ZwTimer_InitT2();
	ZwTimer_SetT2(TIMER2_PERIOD);
	ZwTimer_EnableInterruptsT2(TRUE);
}
// -----------------------------------------

void InitializeADC()
{
	ZwADC_Init(ADC_PRESCALER, ADC_CD2, ADC_SH);
}
// -----------------------------------------

void InitializeSCI()
{
	// Initialize and prepare SCI modules
	ZwSCIb_Init(SCIB_BR, SCIB_DB, SCIB_PARITY, SCIB_SB, FALSE);
	ZwSCIb_InitFIFO(16, 0);
	ZwSCIb_EnableInterrupts(FALSE, FALSE);

	ZwSCI_EnableInterruptsGlobal(FALSE);
}
// -----------------------------------------

void InitializeSPI()
{
	// Init master optical transmitter interface
	ZwSPIb_Init(TRUE, SPIB_BAUDRATE, IBP_CHAR_SIZE, SPIB_PLR, SPIB_PHASE, ZW_SPI_INIT_TX, FALSE, FALSE);
	ZwSPIb_InitFIFO(0, 0);
	ZwSPIb_ConfigInterrupts(FALSE, FALSE);
	ZwSPIb_EnableInterrupts(FALSE, FALSE);

	// Init master optical receiver interface
	ZwSPIa_Init(FALSE, 0, IBP_CHAR_SIZE, SPIA_PLR, SPIA_PHASE, ZW_SPI_INIT_RX, FALSE, FALSE);
	ZwSPIa_InitFIFO(IBP_PACKET_SIZE, 0);
	ZwSPIa_ConfigInterrupts(TRUE, FALSE);
	ZwSPIa_EnableInterrupts(TRUE, FALSE);

	// Common (ABCD)
	ZwSPI_EnableInterruptsGlobal(TRUE);
}
// -----------------------------------------

void InitializeCAN()
{
	// Init CAN
	ZwCANa_Init(CANA_BR, CANA_BRP, CANA_TSEG1, CANA_TSEG2, CANA_SJW);

	// Register system handler
	ZwCANa_RegisterSysEventHandler(&CONTROL_NotifyCANFault);

    // Allow interrupts for CAN
    ZwCANa_InitInterrupts(TRUE);
    ZwCANa_EnableInterrupts(TRUE);
}
// -----------------------------------------

void InitializePWM()
{
	ZwPWMB_InitBridge12(CPU_FRQ, PWM_FREQUENCY, 0, 0, 0, PWM_SATURATION);
}
// -----------------------------------------

void InitializeBoard()
{
   	ZbGPIO_Init();
   	ZbMemory_Init();
}
// -----------------------------------------

void InitializeController()
{
	CONTROL_Init();
}
// -----------------------------------------

// ISRs
// -----------------------------------------
#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(Timer0_ISR, "ramfuncs");
	#pragma CODE_SECTION(Timer2_ISR, "ramfuncs");
	#pragma CODE_SECTION(SEQ1_ISR, "ramfuncs");
	#pragma CODE_SECTION(SPIaRX_ISR, "ramfuncs");
#endif
//
#pragma INTERRUPT(Timer0_ISR, HPI);

ISRCALL Timer0_ISR(void)
{
	// Request sample
	SS_GetData(FALSE);

	// allow other interrupts from group 1
	TIMER0_ISR_DONE;
}
// -----------------------------------------

ISRCALL Timer2_ISR(void)
{
	static Int16U dbgCounter = 0;

	// Update time
	++CONTROL_TimeCounter;

	// Service watch-dogs
	if (CONTROL_BootLoaderRequest != BOOT_LOADER_REQUEST)
	{
		ZwSystem_ServiceDog();
		ZbWatchDog_Strobe();
	}

	++dbgCounter;
	if(dbgCounter == DBG_COUNTER_PERIOD)
	{
		ZbGPIO_ToggleLED1();
		dbgCounter = 0;
	}

	// no PIE
	TIMER2_ISR_DONE;
}
// -----------------------------------------

// ADC SEQ1 ISR
ISRCALL SEQ1_ISR(void)
{
	// Handle interrupt
	ZwADC_ProcessInterruptSEQ1();
	// Dispatch results
	ZwADC_Dispatch1();

	// allow other interrupts from group 1
	ADC_ISR_DONE;
}
// -----------------------------------------

// Line 0 ISR
ISRCALL CAN0_ISR(void)
{
    // handle CAN system events
	ZwCANa_DispatchSysEvent();
	// allow other interrupts from group 9
	CAN_ISR_DONE;
}
// -----------------------------------------

ISRCALL SPIaRX_ISR()
{
	// Handle interrupt
	ZwSPIa_ProcessRXInterrupt();
	SS_HandleSlaveTransmission();

	// allow other interrupts from group 6
	SPI_ISR_DONE;
}
// -----------------------------------------

// ILLEGAL ISR
ISRCALL IllegalInstruction_ISR(void)
{
	// Disable interrupts
	DINT;

	// Terminate PWM - force low
	EPwm1Regs.AQCSFRC.bit.CSFA = 1;
	EPwm2Regs.AQCSFRC.bit.CSFA = 1;

	// Reset system using WD
	ZwSystem_ForceDog();
}
// -----------------------------------------
