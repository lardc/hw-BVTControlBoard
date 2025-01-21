#ifndef __JSON_DESCRIPTION_H
#define __JSON_DESCRIPTION_H

#include "Global.h"

#define JSON_POINTERS_SIZE 32

extern Int16U JSONPointers[JSON_POINTERS_SIZE];

void JSON_AssignPointer(Int16U Index, Int32U Pointer);

#endif // __JSON_DESCRIPTION_H
