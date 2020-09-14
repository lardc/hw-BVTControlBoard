// -----------------------------------------
// Declarations of special memory buffers
// ----------------------------------------

#ifndef __MEM_BUFF_H
#define __MEM_BUFF_H

// Include
#include "stdinc.h"
//
#include "Constraints.h"


// Constants
//
#define VALUES_x_SIZE		1000


// Macro
//
#define IND_EP_I			BIT0
#define IND_EP_V			BIT1
#define IND_EP_DBG			BIT2
#define IND_EP_ERR			BIT3
#define IND_EP_PEAK_I		BIT4
#define IND_EP_PEAK_V		BIT5


// Variables
//
extern Int16U MEMBUF_Values_I[];
extern Int16U MEMBUF_Values_V[];
extern Int16U MEMBUF_Values_DIAG[];
extern Int16U MEMBUF_Values_Err[];
extern Int16U MEMBUF_Values_Ipeak[];
extern Int16U MEMBUF_Values_Vpeak[];
//
extern volatile Int16U MEMBUF_ValuesIV_Counter;
extern volatile Int16U MEMBUF_ValuesDIAG_Counter;
extern volatile Int16U MEMBUF_ValuesErr_Counter;
extern volatile Int16U MEMBUF_ValuesIVpeak_Counter;


#endif // __MEM_BUFF_H
