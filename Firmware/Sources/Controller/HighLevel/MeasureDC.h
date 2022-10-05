// -----------------------------------------
// Measuring logic DC
// ----------------------------------------

#ifndef __MEASURE_DC_H
#define __MEASURE_DC_H

// Include
#include "stdinc.h"
//
#include "IQmathLib.h"

// Functions
//
// Start measurement process
Boolean MEASURE_DC_StartProcess(Int16U Type, pInt16U pDFReason, pInt16U pProblem);
// Finish and clean up
void MEASURE_DC_FinishProcess();
// Emergency stop during measurement process
void MEASURE_DC_Stop(Int16U Reason);

#endif // __MEASURE_DC_H
