// -----------------------------------------
// System parameters
// ----------------------------------------
#ifndef __SYSCONFIG_H
#define __SYSCONFIG_H

// Include
#include <ZwBase.h>
#include <BoardConfig.h>

// CPU & System
//--------------------------------------------------------
#define CPU_PLL				10          // OSCCLK * PLL div 2 = CPUCLK: 20 * 10 / 2 = 100
#define CPU_CLKINDIV		0           // "div 2" in the previous equation
#define SYS_HISPCP       	0x01   		// SYSCLKOUT / 2
#define SYS_LOSPCP       	0x01    	// SYSCLKOUT / 2
//--------------------------------------------------------

// Boot-loader
//--------------------------------------------------------
#define BOOT_LOADER_REQUEST	0xABCD
//--------------------------------------------------------

// Power control
//--------------------------------------------------------
#define SYS_PUMOD			ZW_POWER_ADC_CLK  | \
							ZW_POWER_SPIA_CLK | ZW_POWER_SPIB_CLK | \
							ZW_POWER_SPIC_CLK | ZW_POWER_SCIB_CLK | \
							ZW_POWER_PWM1_CLK | ZW_POWER_PWM2_CLK | \
							ZW_POWER_PWM3_CLK | ZW_POWER_PWM4_CLK | \
							ZW_POWER_CANA_CLK

#define SYS_WD_PRESCALER	0x07
//--------------------------------------------------------

// GPIO
//--------------------------------------------------------
// Input filters
#define GPIO_TSAMPLE		200			// T[sample_A] = (1/ 100MHz) * (2 * 200) = 4 uS

// Flash
//--------------------------------------------------------
#define FLASH_FWAIT			3
#define FLASH_OTPWAIT		5
//--------------------------------------------------------

// TIMERs
//--------------------------------------------------------
#define CONTROL_FREQUENCY	20000L		// in Hz

#define CS_T0_FREQ			CONTROL_FREQUENCY
#define CS_T2_FREQ			1000		// in Hz

#define TIMER0_PERIOD		(1000000L / CS_T0_FREQ)
#define TIMER2_PERIOD		(1000000L / CS_T2_FREQ)

#define DBG_FREQ			2			// 2 Hz
#define DBG_COUNTER_PERIOD	(CS_T2_FREQ / (DBG_FREQ * 2))
//--------------------------------------------------------

// SPI-A
//--------------------------------------------------------
#define SPIA_PLR			TRUE		// CLK high in idle state
#define SPIA_PHASE			FALSE
//--------------------------------------------------------

// SPI-B
//--------------------------------------------------------
#define SPIB_BAUDRATE		6250000L  	// SPI clock = 6.25 MHz
#define SPIB_PLR			TRUE		// CLK high in idle state
#define SPIB_PHASE			FALSE
//--------------------------------------------------------

// SRAM
//--------------------------------------------------------
#define MEM_EPROM_BAUDRATE	1000000L	// SPI clock = 1 MHz
#define MEM_SRAM_BAUDRATE	5000000L	// SPI clock = 5 MHz
#define MEM_PLR				FALSE		// CLK low in idle state
#define MEM_PHASE			TRUE
//--------------------------------------------------------

// SCI-B
//--------------------------------------------------------
#define SCIB_BR				115200L		// UART baudrate = 115200 bps
#define SCIB_DB				8			// 8 bit
#define SCIB_SB				FALSE		// 1 stop bit
#define SCIB_PARITY			ZW_PAR_NONE	// No parity
//--------------------------------------------------------

// CAN-A
//--------------------------------------------------------
#define CANA_BR				1000000L
#define CANA_BRP			9
#define CANA_TSEG1			6
#define CANA_TSEG2			1
#define CANA_SJW			1
//--------------------------------------------------------

// ADC
//--------------------------------------------------------
#define ADC_PRESCALER		0			// HSPCLK / (1 * 1) = 50 MHz
#define ADC_CD2				TRUE		// Div ADC core / 2 = 25 MHz
#define ADC_SH				2			// S/H sample window = 2 => 6.25 MSPS
//--------------------------------------------------------

// PWM
//--------------------------------------------------------
#define PWM_FREQUENCY		20000L      // in Hz
#define PWM_TH				1000	    // 1000 ns
#define PWM_SATURATION		((Int16S)(ZW_PWM_DUTY_BASE * 0.95f))
//--------------------------------------------------------

// MISCELLANEOUS
//--------------------------------------------------------
#define MSC_PON_DELAY_MS	500			// 500 ms
#define MSC_PON_SPI_RST		2000		// 2000 ms
//--------------------------------------------------------

#endif // __SYSCONFIG_H
