// ----------------------------------------
// Measuring logic AC
// ----------------------------------------

#ifndef __MEASURE_AC_H
#define __MEASURE_AC_H

// Include
#include "stdinc.h"
//
#include "IQmathLib.h"

// Functions
//
// Start measurement process
Boolean MEASURE_AC_StartProcess(Int16U Type, pInt16U pDFReason, pInt16U pProblem);
// Finish and clean up
void MEASURE_AC_FinishProcess();
// Emergency stop during measurement process
void MEASURE_AC_Stop(Int16U Reason);
void MEASURE_AC_DoSampling();
void MEASURE_AC_CCSub_CorrectionAndLog(Int16S ActualCorrection, Boolean LogOnly);

#endif // __MEASURE_AC_H
