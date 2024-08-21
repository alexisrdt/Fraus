#include "fraus/fonts/reader.h"

FrResult frReadInt8(FrFileReader* pReader, int8_t* pValue)
{
	if(!pReader || !pValue)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	uint8_t value;
	const FrResult result = frReadUint8(pReader, &value);
	if(result != FR_SUCCESS)
	{
		return result;
	}

	// Vulkan imposes twos-complement on host
	*pValue = value;

	return FR_SUCCESS;
}

FrResult frReadUint8(FrFileReader* pReader, uint8_t* pValue)
{
	if(!pReader || !pValue)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(fread(pReader->buffer, 1, 1, pReader->file) != 1)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	*pValue = pReader->buffer[0];

	return FR_SUCCESS;
}

FrResult frReadUint16(FrFileReader* pReader, uint16_t* pValue)
{
	if(!pReader || !pValue)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(fread(pReader->buffer, 1, 2, pReader->file) != 2)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	*pValue = FR_MSBF_TO_U16(pReader->buffer);

	return FR_SUCCESS;
}

FrResult frReadInt16(FrFileReader* pReader, int16_t* pValue)
{
	if(!pReader || !pValue)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	uint16_t value;
	const FrResult result = frReadUint16(pReader, &value);
	if(result != FR_SUCCESS)
	{
		return result;
	}

	// Vulkan imposes twos-complement on host
	*pValue = value;

	return FR_SUCCESS;
}

FrResult frReadUint32(FrFileReader* pReader, uint32_t* pValue)
{
	if(!pReader || !pValue)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(fread(pReader->buffer, 1, 4, pReader->file) != 4)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	*pValue = FR_MSBF_TO_U32(pReader->buffer);

	return FR_SUCCESS;
}

FrResult frReadF2d14(FrFileReader* pReader, float* pValue)
{
	uint16_t value;
	const FrResult result = frReadUint16(pReader, &value);
	if(result != FR_SUCCESS)
	{
		return result;
	}

	*pValue = FR_F2D14_FLOAT(value);

	return FR_SUCCESS;
}

FrResult frSkipBytes(FrFileReader* pReader, long bytes)
{
	if(!pReader)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(fseek(pReader->file, bytes, SEEK_CUR) != 0)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	return FR_SUCCESS;
}

FrResult frMoveTo(FrFileReader* pReader, long offset)
{
	if(!pReader)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(fseek(pReader->file, offset, SEEK_SET) != 0)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	return FR_SUCCESS;
}
