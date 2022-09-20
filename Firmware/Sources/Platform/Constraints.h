// ----------------------------------------
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
// in Vrms
#define START_VOLTAGE_MIN		100
#define START_VOLTAGE_MAX		1000
#define START_VOLTAGE_DEF		500

// in kV/sec x10
#define VOLTAGE_RATE_MIN		1
#define VOLTAGE_RATE_MAX		30
#define VOLTAGE_RATE_DEF		10

#define K_REG_MAX				100

// in Vrms
#define FE_ABS_MAX				1000
#define FE_ABS_DEF				100

// in %
#define FE_REL_MAX				100
#define FE_REL_DEF				10

#define FE_COUNTER_MAX			10
#define FE_COUNTER_DEF			3

// in X:1
#define TRANS_COEFF_MIN			50
#define TRANS_COEFF_MAX			150
#define TRANS_COEFF_DEF			100

// in Vdc
#define PRIM_VOLTAGE_MIN		1
#define PRIM_VOLTAGE_MAX		200
#define PRIM_VOLTAGE_DEF		150

// in Vrms
#define TARGET_VOLTAGE_MIN		1000
#define TARGET_VOLTAGE_MAX		10500
#define TARGET_VOLTAGE_DEF		1000

// in Irms mA part
#define ILIMIT_mA_MAX			100
#define ILIMIT_mA_DEF			10

// in Irms uA part
#define ILIMIT_uA_MAX			999

// in sec
#define VOLTAGE_PLATE_TIME_MIN	1
#define VOLTAGE_PLATE_TIME_MAX	60
#define VOLTAGE_PLATE_TIME_DEF	1

#define SCOPE_RATE_MAX			100		// skip 99 point, write 1
#define SCOPE_RATE_DEF			0		// write all

#define X_D_DEF0				10
#define X_D_DEF1				100
#define X_D_DEF2				1000
#define X_D_DEF3				10000

// Variables
//
extern const TableItemConstraint NVConstraint[DATA_TABLE_NV_SIZE];
extern const TableItemConstraint VConstraint[DATA_TABLE_WP_START - DATA_TABLE_WR_START];

#endif // __CONSTRAINTS_H
