#include "fraus/fonts/reader.h"

FrResult frReadInt8(FrFileReader* const reader, int8_t* const value)
{

	uint8_t valueTemp;
	const FrResult result = frReadUint8(reader, &valueTemp);
	if(result != FR_SUCCESS)
	{
		return result;
	}

	*value = valueTemp;

	return FR_SUCCESS;
}

FrResult frReadUint8(FrFileReader* const reader, uint8_t* const value)
{
	if(fread(reader->buffer, 1, 1, reader->file) != 1)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	*value = reader->buffer[0];

	return FR_SUCCESS;
}

FrResult frReadUint16(FrFileReader* const reader, uint16_t* const value)
{
	if(fread(reader->buffer, 1, 2, reader->file) != 2)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	*value = FR_MSBF_TO_U16(reader->buffer);

	return FR_SUCCESS;
}

FrResult frReadInt16(FrFileReader* const reader, int16_t* const value)
{
	uint16_t valueTemp;
	const FrResult result = frReadUint16(reader, &valueTemp);
	if(result != FR_SUCCESS)
	{
		return result;
	}

	*value = valueTemp;

	return FR_SUCCESS;
}

FrResult frReadUint32(FrFileReader* const reader, uint32_t* const value)
{
	if(fread(reader->buffer, 1, 4, reader->file) != 4)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	*value = FR_MSBF_TO_U32(reader->buffer);

	return FR_SUCCESS;
}

FrResult frReadF2d14(FrFileReader* const reader, float* const value)
{
	uint16_t valueTemp;
	const FrResult result = frReadUint16(reader, &valueTemp);
	if(result != FR_SUCCESS)
	{
		return result;
	}

	*value = FR_F2D14_FLOAT(valueTemp);

	return FR_SUCCESS;
}

FrResult frSkipBytes(FrFileReader* const reader, const long bytes)
{
	if(fseek(reader->file, bytes, SEEK_CUR) != 0)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	return FR_SUCCESS;
}

FrResult frMoveTo(FrFileReader* const reader, const long offset)
{
	if(fseek(reader->file, offset, SEEK_SET) != 0)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	return FR_SUCCESS;
}
