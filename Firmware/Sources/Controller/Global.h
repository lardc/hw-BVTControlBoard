﻿// -----------------------------------------
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

// Regulator parameters
#define PWM_REDUCE_RATE			50				// in ticks per regulator cycle
#define PWM_SKIP_NEG_PULSES		TRUE			// Skip measurement at negative primary pulses
#define PWM_INVERT_POLARITY		FALSE			// PWM global polarity inversion

// --- Debug modes
#define DBG_USE_BRIDGE_SHORT	TRUE
#define DBG_USE_OPTO_TIMEOUT	TRUE
#define DBG_USE_FOLLOWING_ERR	TRUE

// Invert result values
#define SCOPE_DATA_INVERT		TRUE
// ----------------------------------------

// --- Capacitor battery
#define CAP_VOLTAGE_DELTA		10				// Detection voltage delta (in %)
#define CAP_POW_VOLT_MARGIN		20				// Power and voltage margin for primary side (in %)
#define BAT_CHARGE_TIMEOUT		15000			// in ms
// ----------------------------------------

// --- Measure AC section
// First pulse max current
#define MAX_CURRENT_1ST_PULSE	_IQ(25.0f)		// in mA

// Modes for HVDigitizer
#define HVD_VL_TH				_IQ(1000)		// < 1000V		(low range)
#define HVD_IL_DCM_TH			_IQ(5.0f)		// <= 5mA		(low range)
#define HVD_IL_TH				_IQ(30.0f)		// <= 30mA		(low range)
#define HVD_IH_TH				_IQ(500.0f)		// <= 500mA		(high range)

// Pre-plate parameters
#define PRE_PLATE_MAX_TIME		300				// pre-plate max time for stabilization (in ms)
#define PRE_PLATE_MAX_ERR		_IQ(10.0f)		// pre-plate max voltage error (in V)

// --- Measure DC section
// Resistance limits
#define RES_LIMIT_LOW			10				// in MOhm
#define RES_LIMIT_HIGH			999				// in MOhm

// Regulator parameters
#define CTRL_VOLT_TO_PWM_DIV	10				// Divisor to convert voltage to PWM
#define CTRL_KI_SATURATION		_IQ(1000)

// Modes for HVDigitizer
#define HVD_DC_VL_TH			_IQ(500)		// < 500V		(low range)
#define HVD_DC_IL_TH			_IQ(100)		// <= 100uA		(low range)
#define HVD_DC_IH_TH			_IQ(5000)		// <= 5000uA	(high range)

// Avg sampling period
#define DC_PLATE_SMPL_TIME		500				// in us

// --- Measure resistance section
#define DC_RES_VPLATE			1000			// in ms
// ----------------------------------------

// Following error settings
// --- AC mode
#define FE_MAX_ABSOLUTE			_IQ(500)		// in V
#define FE_MAX_FRACTION			_IQ(0.2f)		// part of 1
#define FE_MAX_COUNTER			3
// --- DC mode
#define FE_DC_MAX_FRACTION		_IQ(0.1f)		// part of 1 for DC mode
#define FE_DC_MIN_VOLTAGE		_IQ(200.0f)		// in V
#define FE_DC_MAX_COUNTER		20
// ----------------------------------------

// Data logger
#define DIAG_CURRENT_MUL		1000			// diag current logger multiplier
#define HV_SWITCH_DELAY			100000L			// time delay for HV commutation

#endif // __GLOBAL_H
