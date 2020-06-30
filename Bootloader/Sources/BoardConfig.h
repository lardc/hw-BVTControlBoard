// -----------------------------------------
// Board parameters
// ----------------------------------------

#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

// Include
#include <ZwBase.h>

// Program build mode
//
#define BOOT_ALT_ADDRESS
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
#define HWUSE_SCI_A

// IO debug LED
#define DBG_LED				3

// IO placement

#define SCI_A_QSEL			GPAQSEL2
#define SCI_A_MUX			GPAMUX2
#define SCI_A_RX			GPIO28
#define SCI_A_TX			GPIO29
#define SCI_A_MUX_SELECTOR	1



#endif // __BOARD_CONFIG_H
