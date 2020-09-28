// -----------------------------------------
// Measuring logic test
// ----------------------------------------

#ifndef __MEASURE_TEST_H
#define __MEASURE_TEST_H

// Include
#include "stdinc.h"
//
#include "IQmathLib.h"

// Functions
//
// Start measurement process
Boolean MEASURE_TEST_StartProcess(Int16U Type, pInt16U pDFReason, pInt16U pProblem);
// Finish and clean up
void MEASURE_TEST_FinishProcess();
// Emergency stop during measurement process
void MEASURE_TEST_Stop(Int16U Reason);

#endif // __MEASURE_TEST_H
