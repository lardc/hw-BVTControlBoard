// ----------------------------------------
// Declarations of special memory buffers
// ----------------------------------------

// Header
#include "MemoryBuffers.h"

// Variables
//
Int16U MEMBUF_Values_I[VALUES_x_SIZE];
Int16U MEMBUF_Values_V[VALUES_x_SIZE];
Int16U MEMBUF_Values_Err[VALUES_x_SIZE];
Int16U MEMBUF_Values_DIAG[VALUES_x_SIZE];
Int16U MEMBUF_Values_Ipeak[VALUES_x_SIZE];
Int16U MEMBUF_Values_Vpeak[VALUES_x_SIZE];
Int16U CONTROL_DiagData[VALUES_DIAG_SIZE];
//
volatile Int16U MEMBUF_ValuesIV_Counter = 0;
volatile Int16U MEMBUF_ValuesDIAG_Counter = 0;
volatile Int16U MEMBUF_ValuesErr_Counter = 0;
volatile Int16U MEMBUF_ValuesIVpeak_Counter = 0;
