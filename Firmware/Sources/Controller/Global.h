// ----------------------------------------
// Global definitions
// ----------------------------------------

#ifndef __GLOBAL_H
#define __GLOBAL_H

// Include
#include "stdinc.h"
#include "IQmathLib.h"

// Constants
// Global miscellaneous parameters
#define	SCCI_TIMEOUT_TICKS		1000			// in ms
#define DT_EPROM_ADDRESS		0
#define EP_COUNT				8
#define VALUES_x_SIZE			500

// Regulator parameters
#define PWM_REDUCE_RATE			50				// in ticks per regulator cycle

// Following error settings
#define FE_MAX_ABSOLUTE			_IQ(500)		// in V
#define FE_MAX_FRACTION			_IQ(0.2f)		// part of 1
#define FE_MAX_COUNTER			3

#endif // __GLOBAL_H
