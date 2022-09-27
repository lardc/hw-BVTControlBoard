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
#define	SCCI_TIMEOUT_TICKS			1000 // in ms
#define DT_EPROM_ADDRESS			0
#define EP_COUNT					9
#define VALUES_x_SIZE				500

// RMS current range limits (in mA)
#define I_RANGE_LOW					_IQ(1)
#define I_RANGE_MID					_IQ(10)
#define I_RANGE_HIGH				_IQ(100)

// Допустимое отклонение напряжения первичной стороны (в %)
#define PRIM_V_MAX_DELTA			5

// Параметры остановки / снижения ШИМ
#define PWM_MIN_REDUCE_RATE			50	// минимальная скорость снижения ШИМ в тиках
#define PWM_REDUCE_RATE_MAX_STEPS	4	// максимум шагов снижения

// Параметры обработки КЗ
#define SC_VOLTAGE_THR				_IQ(400)	// Максимальное напряжение при КЗ

#endif // __GLOBAL_H
