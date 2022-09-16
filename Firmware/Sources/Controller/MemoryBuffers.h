// ----------------------------------------
// Declarations of special memory buffers
// ----------------------------------------

#ifndef __MEM_BUFF_H
#define __MEM_BUFF_H

// Include
#include "stdinc.h"
#include "Constraints.h"

// Constants
#define VALUES_x_SIZE		1000

// Variables
extern Int16U MEMBUF_Values_V[];
extern Int16U MEMBUF_Values_ImA[];
extern Int16U MEMBUF_Values_IuA[];
extern Int16U MEMBUF_Values_PWM[];
extern Int16U MEMBUF_Values_Err[];
//
extern volatile Int16U MEMBUF_ValuesVI_Counter;
extern volatile Int16U MEMBUF_ValuesPWM_Counter;
extern volatile Int16U MEMBUF_ValuesErr_Counter;

#endif // __MEM_BUFF_H
