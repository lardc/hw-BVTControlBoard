// ----------------------------------------
// Board parameters
// ----------------------------------------

#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

// Include
#include <ZwBase.h>

// Program build mode
//
#define BOOT_FROM_FLASH					// normal mode
#define RAM_CACHE_SPI_ABCD				// cache SPI-A(BCD) functions

// Board options
#define OSC_FRQ				(20MHz)			// on-board oscillator
#define CPU_FRQ_MHZ			100				// CPU frequency = 100MHz
#define CPU_FRQ				(CPU_FRQ_MHZ * 1000000L) 
#define SYS_HSP_FREQ		(CPU_FRQ / 2) 	// High-speed bus frequency = 50MHz
#define SYS_LSP_FREQ		(CPU_FRQ / 2) 	// Low-speed bus frequency = 50MHz
//
#define ZW_PWM_DUTY_BASE	5000

// Peripheral options
#define HWUSE_SPI_A
#define HWUSE_SPI_B
#define HWUSE_SPI_C
#define HWUSE_SCI_B

// EPROM memory bank address
#define SRAM_BANK_COUNT				7
#define SRAM_BANK_CAPACITY_SAMPLE	64000 // 16bit words
#define MEM_BANK_EPROM				7

// IO placement
#define SPI_A_QSEL		    GPAQSEL2
#define SPI_A_MUX			GPAMUX2
#define SPI_A_SIMO			GPIO16
#define SPI_A_SOMI			GPIO17
#define SPI_A_CLK			GPIO18
#define SPI_A_CS			GPIO19
//
#define SPI_B_QSEL			GPAQSEL2
#define SPI_B_MUX			GPAMUX2
#define SPI_B_SIMO			GPIO24
#define SPI_B_SOMI			GPIO25
#define SPI_B_CLK			GPIO26
#define SPI_B_CS			GPIO27
//
#define SPI_C_QSEL			GPAQSEL2
#define SPI_C_MUX			GPAMUX2
#define SPI_C_SIMO			GPIO20	
#define SPI_C_SOMI			GPIO21		
#define SPI_C_CLK			GPIO22
#define SPI_C_CS			GPIO23
//
#define SCI_B_QSEL			GPAQSEL1
#define SCI_B_MUX			GPAMUX1
#define SCI_B_RX			GPIO11
#define SCI_B_TX			GPIO9
#define SCI_B_MUX_SELECTOR	2
//
#define PIN_LED_1			6
#define PIN_POWER_EN1		8
#define PIN_POWER_EN2		13
#define PIN_POWER_EN3		17
#define PIN_WD_RST			32
#define PIN_OUT_SYNC		33
#define PIN_MEM_A			10
#define PIN_MEM_B			25
#define PIN_MEM_C			7
#define PIN_TFAULT			15
#define PIN_SHORT			12
#define PIN_STOP			14
#define PIN_EXT_IND			27

// ADC placement
#define AIN_V_CAP			0x08	// 1000b = B IN0
//
#define AIN_LO				0x0D	// 1101b = B + IN5

#endif // __BOARD_CONFIG_H
