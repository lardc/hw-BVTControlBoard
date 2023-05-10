// ----------------------------------------
// Program entry point
// ----------------------------------------

// Include
//
#include <stdinc.h>
//
#include "ZwDSP.h"
#include "SysConfig.h"
#include "Controller.h"

// FORWARD FUNCTIONS
// ----------------------------------------
Boolean InitializeCPU();
void InitializeBoard();
void InitializeTimers();
void InitializeSCI();
void InitializeCAN();
void InitializeController();
void FlashLoader(void);
// ----------------------------------------

// FORWARD ISRs
// ----------------------------------------
// CPU Timer 2 ISR
ISRCALL Timer2_ISR();
// CAN Line 0 ISR
ISRCALL CAN0_ISR();
// ILLEGAL ISR
ISRCALL IllegalInstruction_ISR();
// ----------------------------------------

// FUNCTIONS
// ----------------------------------------
// Program main function
void main()
{
#ifdef BOOT_ALT_ADDRESS
	// Redefine pointer
	PrgAdrStart = ((Uint16*)0x3E8000);
#endif
	
	// Set request flag if no firmware
	if(*PrgAdrStart == 0xFFFF || *PrgWrFlag == BOOT_LOADER_REQUEST)
		WaitForFWUpload = TRUE;
	
	FlashLoader();
}
// ----------------------------------------

void FlashLoader(void)
{
	// Boot process
	InitializeCPU();
	InitializeBoard();
	InitializeTimers();
	InitializeSCI();
	InitializeCAN();
	
	// LED init
	ZwGPIO_PinToOutput(DBG_LED);
	
	// Setup ISRs
	BEGIN_ISR_MAP
	ADD_ISR(TINT2, Timer2_ISR);
	ADD_ISR(ECAN0INTA, CAN0_ISR);
	ADD_ISR(ILLEGAL, IllegalInstruction_ISR);
	END_ISR_MAP
	
	// Initialize controller logic
	InitializeController();
	
	// Enable interrupts
	EINT;
	ERTM;
	
	FLASH_Init();
	ZwTimer_StartT2();
	
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
	ZW_FLASH_OPTIMIZE(FLASH_FWAIT, FLASH_OTPWAIT);
	
	return clockInitResult;
}
// ----------------------------------------

// GPIO init
void InitializeBoard()
{
	// Power supply control
	ZwGPIO_PinToOutput(8);
	ZwGPIO_PinToOutput(13);
	ZwGPIO_PinToOutput(17);
	//
	ZwGPIO_WritePin(8, FALSE);
	ZwGPIO_WritePin(13, FALSE);
	ZwGPIO_WritePin(17, TRUE);
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

// Initialize CPU timers
void InitializeTimers()
{
	ZwTimer_InitT2();
	ZwTimer_SetT2(TIMER2_PERIOD);
	ZwTimer_EnableInterruptsT2(TRUE);
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
#pragma CODE_SECTION(Timer2_ISR, "ramfuncs");
#pragma CODE_SECTION(CAN0_ISR, "ramfuncs");
#pragma CODE_SECTION(IllegalInstruction_ISR, "ramfuncs");
#endif

ISRCALL Timer2_ISR(void)
{
	static Int32U Blink = 0;
	
	// Update time
	CONTROL_TimeCounter++;
	Blink++;
	
	// Debug LED blinking
	if(Blink > TIMER2_BLINK_PERIOD)
	{
		ZwGPIO_TogglePin(DBG_LED);
		Blink = 0;
	}

	// no PIE
	TIMER2_ISR_DONE;
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

// ILLEGAL ISR
ISRCALL IllegalInstruction_ISR(void)
{
	// Disable interrupts
	DINT;
	
	// Reset system using WD
	ZwSystem_ForceDog();
}
// ----------------------------------------


