#include "JSONDescription.h"
#include "FormatOutputJSON.h"

const char TemplateJSON[] = "[{ \"param\" : \"Vgt\", \"type\" : \"measure\", \"ranges\": [ { \"rangeId\" : \"1\", \"unitsMultiply\" : \"1000\", \"active\" : \"$\", \"min\": \"$\", \"max\": \"$\" }, { \"rangeId\" : \"2\", \"unitsMultiply\" : \"1000\", \"active\" : \"$\", \"min\": \"$\", \"max\": \"$\" }] }, { \"param\" : \"Igt\", \"type\" : \"set\", \"ranges\": [ { \"rangeId\" : \"3\", \"unitsMultiply\" : \"1\", \"active\" : \"$\", \"min\": \"$\", \"max\": \"$\" }, { \"rangeId\" : \"4\", \"unitsMultiply\" : \"1\", \"active\" : \"$\", \"min\": \"$\", \"max\": \"$\" }, { \"rangeId\" : \"5\", \"unitsMultiply\" : \"1\", \"active\" : \"$\", \"min\": \"$\", \"max\": \"$\" }] }]";

Int16U JSONPointers[JSON_POINTERS_SIZE] = {0};

void JSON_AssignPointer(Int16U Index, Int32U Pointer)
{
	if (Index < JSON_POINTERS_SIZE)
		JSONPointers[Index] = Pointer;
}

