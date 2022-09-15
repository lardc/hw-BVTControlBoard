// ----------------------------------------
// IQ Math utils
// ----------------------------------------

#ifndef __IQMATH_UTIL_H
#define __IQMATH_UTIL_H

// Include
#include "stdinc.h"
#include "IQmathLib.h"

// Macro
#define _IQI(A)			_IQmpyI32(_IQ(1), A)

// Functions
inline _iq _FPtoIQ2(Int32S N, Int32S D)
{
	return _IQdiv(_IQI(N), _IQI(D));
}

#endif // __IQMATH_UTIL_H
