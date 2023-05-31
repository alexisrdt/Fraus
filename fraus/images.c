#include "images.h"

#include <stdio.h>

FrResult fsLoadPng(const char* path)
{
	FILE* file = fopen(path, "rb");
	if(!file) return FR_ERROR_FILE_NOT_FOUND; // TODO: look at errno

	if(!fclose(file)) return FR_ERROR_UNKNOWN; // TODO: look at doc

	return FR_SUCCESS;
}
