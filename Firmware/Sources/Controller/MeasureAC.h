﻿// ----------------------------------------
// Measuring logic AC
// ----------------------------------------

#ifndef __MEASURE_AC_H
#define __MEASURE_AC_H

// Include
#include "stdinc.h"

// Types
typedef enum __ProcessBreakReason
{
	PBR_None = 0,
	PBR_CurrentLimit,
	PBR_RequestFastStop,
	PBR_RequestSoftStop,
	PBR_FollowingError,
	PBR_PWMSaturation,
	PBR_CurrentSaturation
} ProcessBreakReason;

// Functions
Boolean MAC_StartProcess();
void MAC_RequestStop(ProcessBreakReason Reason);

#endif // __MEASURE_AC_H