// ----------------------------------------
// Declarations of special memory buffers
// ----------------------------------------

// Header
#include "MemoryBuffers.h"

// Variables
//
Int16U MEMBUF_Values_V[VALUES_x_SIZE];
Int16U MEMBUF_Values_ImA[VALUES_x_SIZE];
Int16U MEMBUF_Values_IuA[VALUES_x_SIZE];
Int16U MEMBUF_Values_PWM[VALUES_x_SIZE];
Int16U MEMBUF_Values_Err[VALUES_x_SIZE];
//
volatile Int16U MEMBUF_ValuesVI_Counter = 0;
volatile Int16U MEMBUF_ValuesPWM_Counter = 0;
volatile Int16U MEMBUF_ValuesErr_Counter = 0;
