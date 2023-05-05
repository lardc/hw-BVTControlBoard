// ----------------------------------------
// Implements FIR filter
// ----------------------------------------

#ifndef __FIR_FILTER_H
#define __FIR_FILTER_H

// Include
#include "stdinc.h"
//
#include "IQmathLib.h"

// Functions
//
void FIR_Reset();
void FIR_LoadValues(_iq V, _iq I);
void FIR_Apply(_iq *V, _iq *I);

#endif // __FIR_FILTER_H
