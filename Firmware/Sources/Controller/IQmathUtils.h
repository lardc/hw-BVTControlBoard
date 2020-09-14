// -----------------------------------------
// IQ Math utils
// ----------------------------------------

#ifndef __IQMATH_UTIL_H
#define __IQMATH_UTIL_H

// Include
#include "stdinc.h"
//
#include "IQmathLib.h"


// Types
//
typedef struct __FPair
{
	Int16S N, D;
} FPair;


// Macro
//
//
#define IQ_INVOKE(iqn, func)	_IQ##iqn##func
#define _IQI(A)					_IQmpyI32(_IQ(1), A)
#define _IQIn(A, N)				(IQ_INVOKE(N, mpyI32)(_IQ##N(1), A))


// Functions
//
// Calculate log of number
_iq16 _IQ16log(_iq16 x);
// Convert IQ number to FP representation form
inline FPair _IQtoFP(_iq x);


// Inline function
//
// Convert FP form of number to IQ number
inline _iq _FPtoIQ(FPair fp)
{
	return _IQdiv(_IQI(fp.N), _IQI(fp.D));
}
//
// Convert FP form of number to IQ number
inline _iq _FPtoIQ2(Int16S N, Int16S D)
{
	return _IQdiv(_IQI(N), _IQI(D));
}

#endif // __IQMATH_UTIL_H
