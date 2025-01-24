#include "JSONDescription.h"
#include "FormatOutputJSON.h"

const char TemplateJSON[] = "[{\n"
"	'param' : 'Vgt',\n"
"	'type' : 'measure',\n"
"	'ranges': [\n"
"		{\n"
"			'rangeId' : '1',\n"
"			'unitsMultiply' : '1000'\n"
"			'active' : '$',\n"
"			'min': '$',\n"
"			'max': '$'\n"
"		},\n"
"		{\n"
"			'rangeId' : '2',\n"
"			'unitsMultiply' : '1000',\n"
"			'active' : '$',\n"
"			'min': '$',\n"
"			'max': '$'\n"
"		}]\n"
"},\n"
"{\n"
"	'param' : 'Igt',\n"
"	'type' : 'set',\n"
"	'ranges': [\n"
"		{\n"
"			'rangeId' : '3',\n"
"			'unitsMultiply' : '1',\n"
"			'active' : '$',\n"
"			'min': '$',\n"
"			'max': '$'\n"
"		},\n"
"		{\n"
"			'rangeId' : '4',\n"
"			'unitsMultiply' : '1',\n"
"			'active' : '$',\n"
"			'min': '$',\n"
"			'max': '$'\n"
"		},\n"
"		{\n"
"			'rangeId' : '5',\n"
"			'unitsMultiply' : '1',\n"
"			'active' : '$',\n"
"			'min': '$',\n"
"			'max': '$'\n"
"		}]\n"
"}]";

Int16U JSONPointers[JSON_POINTERS_SIZE] = {0};

void JSON_AssignPointer(Int16U Index, Int32U Pointer)
{
	if (Index < JSON_POINTERS_SIZE)
		JSONPointers[Index] = Pointer;
}

