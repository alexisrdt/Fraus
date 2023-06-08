#include "inflate_temp.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FR_LSBF_TO_U16(bytes) \
((uint16_t)((bytes)[0]) | ((uint16_t)((bytes)[1]) << 8))

typedef struct FrInflateIterator
{
	const uint8_t* data;
	size_t size;
	size_t iterator;
} FrInflateIterator;

typedef struct FrInflateData
{
	FrInflateIterator iterator;
	uint8_t final;
	uint8_t* result;
	size_t result_iterator;
} FrInflateData;

typedef struct FrInflateTableEntry
{
	uint16_t symbol;
	uint16_t length;
} FrInflateTableEntry;

static FrResult frNextBit(FrInflateIterator* iterator, uint8_t* bit)
{
	if(!iterator->size)
	{
		perror("Asking for next bit with no more input");
		return FR_ERROR_CORRUPTED_FILE;
	}

	*bit = ((*iterator->data) & (1 << iterator->iterator)) >> iterator->iterator;

	++iterator->iterator;

	if(iterator->iterator == 8)
	{
		iterator->iterator = 0;
		++iterator->data;
		--iterator->size;
	}

	return FR_SUCCESS;
}

static void frFinishByte(FrInflateIterator* iterator)
{
	if(iterator->iterator)
	{
		iterator->iterator = 0;
		++iterator->data;
		--iterator->size;
	}
}

static FrResult frLSBFBits(FrInflateIterator* iterator, uint8_t count, uint16_t* result)
{
	*result = 0;

	uint8_t next_bit;
	for(uint8_t i = 0; i < count; ++i)
	{
		if(frNextBit(iterator, &next_bit) != FR_SUCCESS)
		{
			perror("Could not get next bit");
			return FR_ERROR_CORRUPTED_FILE;
		}

		*result |= (uint16_t)next_bit << i;
	}

	return FR_SUCCESS;
}

static FrResult frMSBFBits(FrInflateIterator* iterator, uint8_t count, uint16_t* result)
{
	*result = 0;

	uint8_t next_bit;
	while(count--)
	{
		if(frNextBit(iterator, &next_bit) != FR_SUCCESS)
		{
			perror("Could not get next bit");
			return FR_ERROR_CORRUPTED_FILE;
		}

		*result = (*result << 1) | next_bit;
	}

	return FR_SUCCESS;
}

static FrResult frInflateBlock(FrInflateData* data)
{
	// Final block flag
	if(frNextBit(&data->iterator, &data->final) != FR_SUCCESS)
	{
		perror("Could not get final block bit.\n");
		return FR_ERROR_CORRUPTED_FILE;
	}
	printf("Final: %s\n", data->final ? "yes" : "no");

	// Block type
	uint16_t block_type;
	if(frLSBFBits(&data->iterator, 2, &block_type) != FR_SUCCESS)
	{
		perror("Could not get block type");
		return FR_ERROR_CORRUPTED_FILE;
	}
	printf("Block type: %u\n", block_type);

	switch(block_type)
	{
		case 0:
		{
			printf("Not compressed\n");

			frFinishByte(&data->iterator);
			if(data->iterator.size < 4)
			{
				perror("Not enough data");
				return FR_ERROR_CORRUPTED_FILE;
			}

			uint16_t length = FR_LSBF_TO_U16(data->iterator.data);
			if(length != ~FR_LSBF_TO_U16(data->iterator.data + 2) || length > data->iterator.size - 4)
			{
				perror("Incorrect length");
				return FR_ERROR_CORRUPTED_FILE;
			}

			memcpy(data->result + data->result_iterator, data->iterator.data + 4, length);
			data->result_iterator += length;

			data->iterator.data += length + 4;
			data->iterator.size -= length + 4;
			break;
		}

		case 1:
			printf("Fixed codes\n");
			break;

		case 2:
		{
			printf("Dynamic codes\n");

			uint16_t literal_length_count, distance_count, length_count;
			if(frLSBFBits(&data->iterator, 5, &literal_length_count) != FR_SUCCESS || literal_length_count > 29)
			{
				perror("Could not read literal/length codes count");
				return FR_ERROR_CORRUPTED_FILE;
			}
			literal_length_count += 257;
			printf("Literal/length codes count: %u.\n", literal_length_count);
			if(frLSBFBits(&data->iterator, 5, &distance_count) != FR_SUCCESS || distance_count > 31)
			{
				perror("Could not read distance codes count");
				return FR_ERROR_CORRUPTED_FILE;
			}
			distance_count += 1;
			printf("Distance codes count: %u.\n", distance_count);
			if(frLSBFBits(&data->iterator, 4, &length_count) != FR_SUCCESS || length_count > 15)
			{
				perror("Could not read code length codes count");
				return FR_ERROR_CORRUPTED_FILE;
			}
			length_count += 4;
			printf("Code length codes count: %u.\n", length_count);

			break;
		}

		default:
			perror("Invalid block type");
			return FR_ERROR_CORRUPTED_FILE;
	}

	return FR_SUCCESS;
}

FrResult frInflate(const uint8_t* pData, size_t size, uint8_t** ppResult)
{
	FrInflateData data = {
		.iterator = {
			.data = pData,
			.size = size
		},
		.result = (uint8_t*)malloc(0xFFFFFF)
	};
	if(!data.result)
	{
		perror("Could not allocate memory for result");
		return FR_ERROR_OUT_OF_MEMORY;
	}

	do
	{
		if(frInflateBlock(&data) != FR_SUCCESS)
		{
			perror("Could not inflate block");
			return FR_ERROR_CORRUPTED_FILE;
		}
	} while(!data.final);
	if(data.iterator.size)
	{
		perror("Final block flag set but input not read entirely");
		return FR_ERROR_CORRUPTED_FILE;
	}

	*ppResult = data.result;

	return FR_SUCCESS;
}
