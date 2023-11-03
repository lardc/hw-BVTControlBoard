// ----------------------------------------
// Global definitions
// ----------------------------------------

#ifndef __GLOBAL_H
#define __GLOBAL_H

// Include
#include "stdinc.h"
#include "IQmathLib.h"

// Constants
// --- Global miscellaneous parameters
#define	SCCI_TIMEOUT_TICKS		1000			// in ms
#define DT_EPROM_ADDRESS		0
#define EP_COUNT				6

// Password to unlock non-volatile area for write
#define ENABLE_LOCKING			FALSE
#define UNLOCK_PWD_1			1
#define UNLOCK_PWD_2			1
#define UNLOCK_PWD_3			1
#define UNLOCK_PWD_4			1
// ----------------------------------------

// Debug modes
#define DBG_USE_OPTO_TIMEOUT	TRUE
#define DBG_USE_FOLLOWING_ERR	TRUE

#define PWM_REDUCE_RATE			50				// Скорость снижения сигнала регулятором (в тиках)
#define SCOPE_DATA_INVERT		TRUE

// First pulse max current
#define MAX_CURRENT_1ST_PULSE	_IQ(25.0f)		// in mA

// Modes for HVDigitizer
#define HVD_VL_TH				_IQ(1000)		// < 1000V
#define HVD_ILL_TH				_IQ(5)			// <= 5mA		(lowest range)
#define HVD_IL_TH				_IQ(30.0f)		// <= 30mA		(low range)
// 310мА выбрано для исключения отсечки на максимальном токе и возможности его корректного измерения
#define HVD_IH_TH				_IQ(310.0f)		// <= 310mA		(high range)

// Pre-plate parameters
#define PRE_PLATE_MAX_TIME		1000			// pre-plate max time for stabilization (in ms)
#define PRE_PLATE_MAX_ERR		_IQ(10.0f)		// pre-plate max voltage error (in V)

// Following error settings
#define FE_MAX_ABSOLUTE			_IQ(500)		// in V
#define FE_MAX_FRACTION			_IQ(0.2f)		// part of 1
#define FE_MAX_COUNTER			3

// Параметры определения КЗ на выходе
#define OUT_SHORT_MAX_V			_IQ(50)			// Максимальное значение напряжения при КЗ (В)
#define OUT_SHORT_MIN_I			_IQ(50)			// Минимальное значение тока при КЗ (мА)

// Capacitor battery
#define CAP_DELTA				5				// Detection voltage delta (in V)
#define CAP_SW_VOLTAGE			3500			// Output voltage switch limit (in V)
#define CAP_SW_POWER			150				// Output power switch limit (in W)
#define BAT_CHARGE_TIMEOUT		15000			// in ms

// Data logger
#define DIAG_CURRENT_MUL		1000			// diag current logger multiplier

#endif // __GLOBAL_H
