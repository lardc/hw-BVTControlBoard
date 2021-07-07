// -----------------------------------------
// Constraints for tunable parameters
// ----------------------------------------

#ifndef __CONSTRAINTS_H
#define __CONSTRAINTS_H

// Include
#include "stdinc.h"
//
#include "DataTable.h"
#include "Global.h"

// Types
//
typedef struct __TableItemConstraint
{
	Int16U Min;
	Int16U Max;
	Int16U Default;
} TableItemConstraint;


// Restrictions
//
#define TEST_CURRENT_MIN		1		// in mA x10
#define TEST_CURRENT_MAX		5000	// in mA x10
#define TEST_CURRENT_DEF		50		// in mA x10
//
#define TEST_CURRENT_DC_MAX		10000	// in uA
//
#define LIMIT_VOLTAGE_MIN		100		// (in V)
#define LIMIT_VOLTAGE_MAX		8000	// (in V)
#define LIMIT_VOLTAGE_DEF		500		// (in V)
//
#define VPLATE_TIME_MIN			100		// (in ms)
#define VPLATE_TIME_MAX			60000	// (in ms)
#define VPLATE_TIME_DEF			500		// (in ms)
//
#define RES_VOLTAGE_MIN			1000	// in V
#define RES_VOLTAGE_MAX			2500	// in V
#define RES_VOLTAGE_DEF			1000	// in V
//
#define RATE_VAC_MIN			1		// (x100 V/s)
#define RATE_VAC_MAX			100		// (x100 V/s)
#define RATE_VAC_DEF			20		// (x100 V/s)
//
#define START_VAC_MIN			LIMIT_VOLTAGE_MIN
#define START_VAC_MAX			1000	// (in V)
#define START_VAC_DEF			200		// (in V)
//
#define VOLTAGE_FREQUENCY_MIN	50		// (in Hz)
#define VOLTAGE_FREQUENCY_MAX	60		// (in Hz)
#define VOLTAGE_FREQUENCY_DEF	50		// (in Hz)
//
#define BRAKE_TIME_MIN			1		// (in ms)
#define BRAKE_TIME_MAX			1000	// (in ms)
#define BRAKE_TIME_DEF			10		// (in ms)
//
#define FAN_TIME_MIN			10		// (in s)
#define FAN_TIME_MAX			600		// (in s)
#define FAN_TIME_DEF			30		// (in s)
//
#define TRANSFORMER_COFF_MIN	10		// 10:1
#define TRANSFORMER_COFF_MAX	200		// 200:1
#define TRANSFORMER_COFF_DEF	100		// 100:1
//
#define NOMINAL_PRIMARY_V_MIN	10		// (in V)
#define NOMINAL_PRIMARY_V_MAX	400		// (in V)
#define NOMINAL_PRIMARY_V_DEF	150		// (in V)
//
#define OPTO_MON_MIN			0
#define OPTO_MON_MAX			50
#define OPTO_MON_DEF			10
//
#define VOLTAGE_DIV_MIN			1		// Pulse rate 1:1
#define VOLTAGE_DIV_MAX			100		// Pulse rate 1:100
#define VOLTAGE_DIV_DEF			1		// Pulse rate 1:1
//
#define SCOPE_RATE_MAX			100		// skip 99 point, write 1
#define SCOPE_RATE_DEF			0		// write all
//
#define LAST_FRAG_SIZE_DEF		1200
//
#define X_D_DEF0				10
#define X_D_DEF1				100
#define X_D_DEF2				1000
#define X_D_DEF3				10000

// Relatively to ADC scale
// Voltage
#define	SVOLTAGE1_COFF_N_DEF	183		//  183 / 1000 = 0.183
#define	SVOLTAGE2_COFF_N_DEF	2015	// 2015 / 1000 = 2.015
#define CAP_V_COFF_N_DEF		37		// 37 / 1000 = 0.037
//
// Current AC mode
#define	SCURRENT1_COFF_N_DEF	765		// (765 / 1000) / 100 = 0.00765
#define	SCURRENT2_COFF_N_DEF	812		// 812 / 10000 = 0.0812
// Current DC mode
#define	SCURRENT_DCL_COFF_N_DEF	25		// 25 / 1000 = 0.025
#define	SCURRENT_DCM_COFF_N_DEF	245		// 245 / 1000 = 0.245


// Variables
//
extern const TableItemConstraint NVConstraint[DATA_TABLE_NV_SIZE];
extern const TableItemConstraint VConstraint[DATA_TABLE_WP_START - DATA_TABLE_WR_START];


#endif // __CONSTRAINTS_H
