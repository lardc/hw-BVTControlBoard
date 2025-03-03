// ----------------------------------------
// Program entry point
// ----------------------------------------

// Include
#include <stdinc.h>
//
#include "ZwDSP.h"
#include "ZbBoard.h"
//
#include "SysConfig.h"
//
#include "Controller.h"
#include "InterboardProtocol.h"
#include "SecondarySampling.h"
#include "Flash.h"


// FORWARD FUNCTIONS
// ----------------------------------------
Boolean InitializeCPU();
void InitializeTimers();
void InitializeADC();
void InitializeSPI();
void InitializeSCI();
void InitializeCAN();
void InitializeBoard();
void InitializeController();
// ----------------------------------------

// FORWARD ISRs
// ----------------------------------------
// CPU Timer 0 ISR
ISRCALL Timer0_ISR();
// CPU Timer 2 ISR
ISRCALL Timer2_ISR();
// CAN Line 0 ISR
ISRCALL CAN0_ISR();
// ADC SEQ1 ISR
ISRCALL SEQ1_ISR();
// EPWM3 TZ ISR
ISRCALL PWM3_TZ_ISR();
// SPI-A RX ISR
ISRCALL SPIaRX_ISR();
// ILLEGAL ISR
ISRCALL IllegalInstruction_ISR();
// ----------------------------------------

// FUNCTIONS
// ----------------------------------------
// Program main function
void main()
{
	Int16U i;

	// Boot process
	InitializeCPU();
	FLASH_Init();

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

	// Setup ISRs
	BEGIN_ISR_MAP
		ADD_ISR(TINT0, Timer0_ISR);
		ADD_ISR(TINT2, Timer2_ISR);
		ADD_ISR(ECAN0INTA, CAN0_ISR);
		ADD_ISR(SEQ1INT, SEQ1_ISR);
		ADD_ISR(EPWM3_TZINT, PWM3_TZ_ISR);
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

	// Do delayed initialization
	CONTROL_DelayedInit();

	// Set watch-dog as WDRST
	ZwSystem_SelectDogFunc(FALSE);
	ZwSystem_EnableDog(SYS_WD_PRESCALER);

	// Start timers
	ZwTimer_StartT2();

	// Re-init SPI
	DELAY_US(MSC_PON_SPI_RST * 1000L);
	CONTROL_ReInitSPI_Rx();

	// Background cycle
	while(TRUE)
		CONTROL_Idle();
}
// ----------------------------------------

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
	ZW_FLASH_MATH_TR_SHADOW;
	ZW_FLASH_OPTIMIZE(FLASH_FWAIT, FLASH_OTPWAIT);

   	return clockInitResult;
}
// ----------------------------------------

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
// ----------------------------------------

void InitializeADC()
{
	// Initialize and prepare ADC
	ZwADC_Init(ADC_PRESCALER, ADC_CD2, ADC_SH);
	ZwADC_ConfigInterrupts(TRUE, FALSE);

	// Enable interrupts on peripheral and CPU levels
	ZwADC_EnableInterrupts(TRUE, FALSE);
	ZwADC_EnableInterruptsGlobal(TRUE);
}
// ----------------------------------------

void InitializeSCI()
{
	// Initialize and prepare SCI modules
	ZwSCIb_Init(SCIB_BR, SCIB_DB, SCIB_PARITY, SCIB_SB, FALSE);
	ZwSCIb_InitFIFO(16, 0);
	ZwSCIb_EnableInterrupts(FALSE, FALSE);

	ZwSCI_EnableInterruptsGlobal(FALSE);
}
// ----------------------------------------

void InitializeSPI()
{
	// Init master optical transmitter interface
	ZwSPIb_Init(TRUE, SPIB_BAUDRATE, 16, SPIB_PLR, SPIB_PHASE, ZW_SPI_INIT_TX, FALSE, FALSE);
	ZwSPIb_InitFIFO(0, 0);
	ZwSPIb_ConfigInterrupts(FALSE, FALSE);
	ZwSPIb_EnableInterrupts(FALSE, FALSE);

	// Init master optical receiver interface
	ZwSPIa_Init(FALSE, 0, 16, SPIA_PLR, SPIA_PHASE, ZW_SPI_INIT_RX, FALSE, FALSE);
	ZwSPIa_InitFIFO(IBP_PACKET_SIZE, 0);
	ZwSPIa_ConfigInterrupts(TRUE, FALSE);
	ZwSPIa_EnableInterrupts(TRUE, FALSE);

	// Common (ABCD)
	ZwSPI_EnableInterruptsGlobal(TRUE);
}
// ----------------------------------------

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
// ----------------------------------------

void InitializeBoard()
{
	// Init board GPIO
   	ZbGPIO_Init();
   	// Init EPROM & SRAM
   	ZbMemory_Init();
}
// ----------------------------------------

void InitializeController()
{
	CONTROL_Init();
}
// ----------------------------------------

// ISRs
// ----------------------------------------
#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(Timer0_ISR, "ramfuncs");
	#pragma CODE_SECTION(Timer2_ISR, "ramfuncs");
	#pragma CODE_SECTION(CAN0_ISR, "ramfuncs");
	#pragma CODE_SECTION(SEQ1_ISR, "ramfuncs");
	#pragma CODE_SECTION(PWM3_TZ_ISR, "ramfuncs");
	#pragma CODE_SECTION(SPIaRX_ISR, "ramfuncs");
	#pragma CODE_SECTION(IllegalInstruction_ISR, "ramfuncs");
#endif
//
#pragma INTERRUPT(Timer0_ISR, HPI);

ISRCALL Timer0_ISR(void)
{
	// Do control cycle
	CONTROL_RealTimeCycle();

	// Handle IBP timeouts
	IBP_HighSpeedTimeoutCycle();

	// allow other interrupts from group 1
	TIMER0_ISR_DONE;
}
// ----------------------------------------

ISRCALL Timer2_ISR(void)
{
	static Int16U dbgCounter = 0;

	// Update low-priority tasks
	CONTROL_UpdateLow();

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
// ----------------------------------------

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
// ----------------------------------------

// Line 0 ISR
ISRCALL CAN0_ISR(void)
{
    // handle CAN system events
	ZwCANa_DispatchSysEvent();
	// allow other interrupts from group 9
	CAN_ISR_DONE;
}
// ----------------------------------------

// EPWM3 TZ ISR
ISRCALL PWM3_TZ_ISR(void)
{
	DINT;

	// Shutdown bridge
	ZwPWMB_SetValue12(0);
	// Notify controller
	CONTROL_RequestStop(DF_BRIDGE_SHORT, TRUE);
	ZwPWM3_ProcessTZInterrupt();

	// allow other interrupts from group 2
	PWM_TZ_ISR_DONE;

	EINT;
}
// ----------------------------------------

ISRCALL SPIaRX_ISR()
{
	// Handle interrupt
	ZwSPIa_ProcessRXInterrupt();
	SS_HandleSlaveTransmission();

	// allow other interrupts from group 6
	SPI_ISR_DONE;
}
// ----------------------------------------

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
// ----------------------------------------


