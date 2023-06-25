#include "inflate_temp.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// LSBF = Least Significant Byte First
// Transforms two bytes in LSBF order into a uint16_t
#define FR_LSBF_TO_U16(bytes) \
((uint16_t)((bytes)[0]) | ((uint16_t)((bytes)[1]) << 8))

/*
 * Iterator over deflate encoded data
 * - data is the data stream
 * - size is the remaining bytes to be read
 * - iterator is an iterator over the bits of the current byte
 */
typedef struct FrInflateIterator
{
	const uint8_t* data;
	size_t size;
	size_t iterator;
} FrInflateIterator;

/*
 * Structure to pass all data useful for inflate algorithm
 * - iterator is the iterator over input data
 * - final is a flag set if the deflate block is the last in the input data stream
 * - result is a buffer to store the inflated result
 * - result_iterator is the number of bytes written to result
 */
typedef struct FrInflateData
{
	FrInflateIterator iterator;
	uint8_t final;
	uint8_t* result;
	size_t result_iterator;
} FrInflateData;

/*
 * Range of symbols whose codes have the same length
 * - lastSymbol is the last symbol in the range
 * - length is the length of associated codes
 */
typedef struct FrInflateRange
{
	uint16_t lastSymbol;
	uint8_t bitLength;
} FrInflateRange;

/*
 * Entry in the sub table
 * - symbol is the symbol associated with the code
 * - bitLength is the length of the sub entry (0 = unused --> error if read, otherwise = symbol)
 */
typedef struct FrInflateSubTableEntry
{
	uint16_t symbol;
	uint8_t bitLength;
} FrInflateSubTableEntry;

/*
 * Entry in the primary level lookup table
 * - pSubTable is the a pointer to the sub table if the code is a prefix for longer codes
 * - symbol is the symbol associated with the code if the code is not a prefix for longer codes
 * - bitLength is the length of the entry (0 = unused --> error if read, > bitLength (maxLength) = sub table, otherwise = symbol)
 */
typedef struct FrInflateTableEntry
{
	union{
		FrInflateSubTableEntry* pSubTable;
		uint16_t symbol;
	};
	uint8_t bitLength;
} FrInflateTableEntry;

// Bit lengths for primary level codes of lookup tables
#define FR_CODE_LENGTH_TABLE_BIT_LENGTH    5
#define FR_LITERAL_LENGTH_TABLE_BIT_LENGTH 9
#define FR_DISTANCE_TABLE_BIT_LENGTH       6

/*
 * Read the next bit in the input data stream if available
 * - iterator is the iterator over the deflate encoded input data
 * - bit is the output in which the value of the next bit will be stored
 */
static FrResult frNextBit(FrInflateIterator* pIterator, uint8_t* pBit)
{
	// If the input has already been entirely read, the last code may still need some bits
	// to match the table bit length, therefore return 0
	if(!pIterator->size)
	{
		*pBit = 0;
		return FR_SUCCESS;
	}

	// Store the value of the bit (right cast to have only two possible values, 0 or 1)
	*pBit = ((*pIterator->data) & (1 << pIterator->iterator)) >> pIterator->iterator;

	// Move the byte iterator by one place
	++pIterator->iterator;

	// If the byte is finished, start a new one
	if(pIterator->iterator == 8)
	{
		pIterator->iterator = 0;
		++pIterator->data;
		--pIterator->size;
	}

	return FR_SUCCESS;
}

/*
 * Finish the input byte being read
 * - iterator is the iterator over the deflate encoded input data
 */
static void frFinishByte(FrInflateIterator* pIterator)
{
	// If the iterator is already pointing to the first bit of a byte, there is nothing to do
	// Otherwise, start the next byte
	if(pIterator->iterator)
	{
		pIterator->iterator = 0;
		++pIterator->data;
		--pIterator->size;
	}
}

/*
 * Read the next input bits as a uint16_t in an LSBF manner (Least Significant Bit First)
 * - iterator is the iterator over the deflate encoded input data
 * - count is the number of bits to read
 * - result is the output in which the value will be stored
 */
static FrResult frLSBFBits(FrInflateIterator* pIterator, uint8_t count, uint16_t* pResult)
{
	// Check that input has not already been entirely read
	if(!pIterator->size) return FR_ERROR_CORRUPTED_FILE;

	// Clear the output
	*pResult = 0;

	uint8_t next_bit;
	for(uint8_t i = 0; i < count; ++i)
	{
		// Read the next bit
		if(frNextBit(pIterator, &next_bit) != FR_SUCCESS) return FR_ERROR_CORRUPTED_FILE;

		// Store it in the i-th place of the result
		*pResult |= (uint16_t)next_bit << i;
	}

	return FR_SUCCESS;
}

/*
 * Read the next input bits as a uint16_t in an MSBF manner (Most Significant Bit First)
 * - iterator is the iterator over the deflate-encoded input data
 * - count is the number of bits to read
 * - result is the output in which the value will be stored
 */
static FrResult frMSBFBits(FrInflateIterator* pIterator, uint8_t count, uint16_t* pResult)
{
	// Check that input has not already been entirely read
	if(!pIterator->size) return FR_ERROR_CORRUPTED_FILE;

	// Clear the output
	*pResult = 0;

	uint8_t next_bit;
	while(count--)
	{
		// Read the next bit
		if(frNextBit(pIterator, &next_bit) != FR_SUCCESS) return FR_ERROR_CORRUPTED_FILE;

		// Append it at the end of the result
		*pResult = (*pResult << 1) | next_bit;
	}

	return FR_SUCCESS;
}

/*
 * Return bits to the input stream
 * - pIterator is the iterator over the deflate-encoded input data
 * - count is the number of bits to return
 */
static void frReturnBits(FrInflateIterator* pIterator, uint8_t count)
{
	div_t division = div(count, 8);
	uint8_t overflow = division.rem > pIterator->iterator ? 1 : 0;

	pIterator->data -= division.quot + overflow;
	pIterator->size += division.quot + overflow;
	pIterator->iterator += 8 * overflow - division.rem;
}

/*
 * Build an inflate lookup table
 * - bitLength is the bit length of table indexes
 * - pRanges is the array of ranges
 * - rangeCount is the number of ranges
 * - pCodes is the array of first codes for each bit length
 * - maxBitLength is the maximum bit length found for this table
 * - ppTable is a pointer to the resulting table
 */
static FrResult frBuildInflateTable(uint8_t bitLength, uint8_t* pSymbolBitLength, uint16_t symbolCount, uint16_t* pLengths, uint8_t maxBitLength, FrInflateTableEntry** ppTable)
{
	// Check arguments
	if(!ppTable || !pSymbolBitLength || !pLengths) return FR_ERROR_INVALID_ARGUMENT;

	// Genreate ranges
	FrInflateRange* pRanges = malloc(symbolCount * sizeof(FrInflateRange));
	if(!pRanges) return FR_ERROR_OUT_OF_MEMORY;
	pRanges[0] = (FrInflateRange){.lastSymbol = 0, .bitLength = pSymbolBitLength[0]};
	uint16_t rangeCount = 0;
	for(uint16_t i = 1; i < symbolCount; ++i)
	{
		if(pSymbolBitLength[i] != pSymbolBitLength[i - 1])
		{
			++rangeCount;
			pRanges[rangeCount].bitLength = pSymbolBitLength[i];
		}

		pRanges[rangeCount].lastSymbol = i;
	}
	++rangeCount;

	// Generate first codes
	uint16_t* next_code = malloc((maxBitLength + 1) * sizeof(uint16_t));
	if(!next_code)
	{
		free(pRanges);
		return FR_ERROR_OUT_OF_MEMORY;
	}
	next_code[0] = 0;
	for(uint8_t i = 1; i <= maxBitLength; ++i)
	{
		next_code[i] = (next_code[i - 1] + pLengths[i - 1]) << 1;
	}

	// Allocate table
	const uint16_t tableSize = UINT16_C(1) << bitLength;
	FrInflateTableEntry* pTable = calloc(tableSize, sizeof(FrInflateTableEntry));
	if(!pTable)
	{
		free(pRanges);
		free(next_code);
		return FR_ERROR_OUT_OF_MEMORY;
	}

	// Build table entries
	uint16_t active_range = 0;
	for(uint16_t i = 0; i <= pRanges[rangeCount - 1].lastSymbol; ++i)
	{
		if(i > pRanges[active_range].lastSymbol) ++active_range;
		while(!pRanges[active_range].bitLength)
		{
			i = pRanges[active_range].lastSymbol + 1;
			++active_range;

			if(i > pRanges[rangeCount - 1].lastSymbol) goto end;
		}
		
		uint16_t code = next_code[pRanges[active_range].bitLength]++;

		// If short code
		if(pRanges[active_range].bitLength <= bitLength)
		{
			code = code << (bitLength - pRanges[active_range].bitLength);
			for(uint16_t j = code; j < code + (UINT16_C(1) << (bitLength - pRanges[active_range].bitLength)); ++j)
			{
				pTable[j] = (FrInflateTableEntry){.bitLength = pRanges[active_range].bitLength, .symbol = i};
			}

			continue;
		}

		// If long code
		const uint16_t prefix = code >> (pRanges[active_range].bitLength - bitLength);
		const uint16_t postfix = (code & ((1 << (pRanges[active_range].bitLength - bitLength)) - 1)) << (maxBitLength - pRanges[active_range].bitLength);
		if(!pTable[prefix].bitLength)
		{
			pTable[prefix].pSubTable = calloc(UINTMAX_C(1) << (maxBitLength - bitLength), sizeof(FrInflateSubTableEntry));
			if(!pTable[prefix].pSubTable)
			{
				for(uint16_t j = 0; j < tableSize - 1; ++j)
				{
					if(pTable[j].bitLength > bitLength) free(pTable[j].pSubTable);
				}
				free(pTable);
				free(pRanges);
				free(next_code);

				return FR_ERROR_OUT_OF_MEMORY;
			}
			pTable[prefix].bitLength = maxBitLength;
		}
		for(uint16_t j = postfix; j < postfix + (1 << (maxBitLength - pRanges[active_range].bitLength)); ++j)
		{
			pTable[prefix].pSubTable[j] = (FrInflateSubTableEntry){.bitLength = pRanges[active_range].bitLength - bitLength, .symbol = i};
		}
	}
	end:

	free(pRanges);
	free(next_code);

	*ppTable = pTable;

	return FR_SUCCESS;
}

/*
 * Free an inflate lookup table
 * - pTable is the table
 * - bitLength is the bit length of table indexes
 */
static void frFreeInflateTable(FrInflateTableEntry* pTable, uint8_t bitLength)
{
	for(int16_t i = (INT16_C(1) << bitLength) - 1; i >= 0; --i)
	{
		if(pTable[i].bitLength > 0 && pTable[i].bitLength <= bitLength) break;

		if(pTable[i].bitLength > bitLength) free(pTable[i].pSubTable);
	}

	free(pTable);
}

/*
 * Reads the next input code from the given lookup table
 * - pIterator is the iterator over the deflate-encoded input data
 * - pTable is the lookup table
 * - bitLength is the bit length of the primary level codes in the lookup table
 * - maxBitLength is the maximum bit length for the table
 * - pSymbol is the output symbol
 */
static FrResult frReadFromTable(FrInflateIterator* pIterator, const FrInflateTableEntry* pTable, uint8_t bitLength, uint8_t maxBitLength, uint16_t* pSymbol)
{
	// Check arguments
	if(!pSymbol || !pIterator || !pTable) return FR_ERROR_INVALID_ARGUMENT;

	// Get code
	uint16_t code;
	if(frMSBFBits(pIterator, bitLength, &code) != FR_SUCCESS) return FR_ERROR_CORRUPTED_FILE;

	// Check invalid code
	if(!pTable[code].bitLength) return FR_ERROR_CORRUPTED_FILE;

	// If short code
	if(pTable[code].bitLength <= bitLength)
	{
		// Set symbol and return extra bits
		*pSymbol = pTable[code].symbol;
		frReturnBits(pIterator, bitLength - pTable[code].bitLength);

		return FR_SUCCESS;
	}

	// If long code
	// No need to check for sub table: if it is a long code, the sub table is guaranteed to exist
	uint16_t subCode;
	if(frMSBFBits(pIterator, maxBitLength - bitLength, &subCode) != FR_SUCCESS) return FR_ERROR_CORRUPTED_FILE;

	// Check invalid sub code
	if(!pTable[code].pSubTable[subCode].bitLength) return FR_ERROR_CORRUPTED_FILE;

	// Set symbol and return extra bits
	*pSymbol = pTable[code].pSubTable[subCode].symbol;
	frReturnBits(pIterator, maxBitLength - bitLength - pTable[code].pSubTable[subCode].bitLength);

	return FR_SUCCESS;
}

/*
 * Inflate a deflate-encoded block
 * - data is the inflate algorithm data
 */
static FrResult frInflateBlock(FrInflateData* pData)
{
	// Read the final block flag
	if(frNextBit(&pData->iterator, &pData->final) != FR_SUCCESS) return FR_ERROR_CORRUPTED_FILE;

	// Read the block type
	uint16_t block_type;
	if(frLSBFBits(&pData->iterator, 2, &block_type) != FR_SUCCESS) return FR_ERROR_CORRUPTED_FILE;

	// Invalid block type
	if(block_type == 3) return FR_ERROR_CORRUPTED_FILE;

	// Raw data block
	if(block_type == 0)
	{
		// Finish the current byte
		frFinishByte(&pData->iterator);

		// Make sure the is enough data to read (4 bytes for LEN and NLEN)
		if(pData->iterator.size < 4) return FR_ERROR_CORRUPTED_FILE;

		// Read LEN
		uint16_t length = FR_LSBF_TO_U16(pData->iterator.data);

		// Make sure NLEN is the ones complement of LEN
		// Also make sure there is at least LEN bytes to read in input data
		if(length != ~FR_LSBF_TO_U16(pData->iterator.data + 2) || length > pData->iterator.size - 4) return FR_ERROR_CORRUPTED_FILE;

		// Copy LEN bytes in the output buffer
		memcpy(pData->result + pData->result_iterator, pData->iterator.data + 4, length);
		pData->result_iterator += length;

		// Update the input data iterator
		pData->iterator.data += length + 4;
		pData->iterator.size -= length + 4;

		return FR_SUCCESS;
	}

	// Encoded block
	FrInflateTableEntry* literal_length_table;
	uint8_t literal_length_max_length = 0;
	const uint8_t literal_length_extra_bits[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
	const uint16_t literal_length_offset[29] = {3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 7, 10, 13, 16, 19, 26, 33, 40, 47, 62, 77, 92, 107, 138, 169, 200, 230};

	FrInflateTableEntry* distance_table;
	uint8_t distance_max_length = 0;
	const uint8_t distance_extra_bits[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
	const uint16_t distance_offset[30] = {1, 1, 1, 1, 1, 2, 3, 6, 9, 16, 23, 38, 53, 84, 115, 178, 241, 368, 495, 750, 1005, 1516, 2027, 3050, 4073, 6120, 8167, 12262, 16357, 24548};

	// Fixed Huffman encoded block
	if(block_type == 1)
	{
		printf("TODO: Fixed codes\n");
		literal_length_table = NULL;
		distance_table = NULL;
	}

	// Dynamic Huffman encoded block
	else
	{
		// Read count of literal/length code lengths
		uint16_t literal_length_count;
		if(frLSBFBits(&pData->iterator, 5, &literal_length_count) != FR_SUCCESS || literal_length_count > 29) return FR_ERROR_CORRUPTED_FILE;
		literal_length_count += 257;

		// Read count of distance code lengths
		uint16_t distance_count;
		if(frLSBFBits(&pData->iterator, 5, &distance_count) != FR_SUCCESS || distance_count > 31) return FR_ERROR_CORRUPTED_FILE;
		distance_count += 1;

		// Read count of code length code lengths
		uint16_t length_count;
		if(frLSBFBits(&pData->iterator, 4, &length_count) != FR_SUCCESS || length_count > 15) return FR_ERROR_CORRUPTED_FILE;
		length_count += 4;

		// Define the order of code length symbols
		const uint16_t code_length_order[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

		// Read and count lengths, find max length
		uint8_t code_length_length[19] = { 0 };
		uint16_t lengths[8] = { 0 };
		uint16_t length;
		uint8_t max_code_length = 0;
		for(uint16_t i = 0; i < length_count; ++i)
		{
			if(frLSBFBits(&pData->iterator, 3, &length) != FR_SUCCESS) return FR_ERROR_CORRUPTED_FILE;

			code_length_length[code_length_order[i]] = (uint8_t)length;

			++lengths[length];

			if(length > max_code_length) max_code_length = (uint8_t)length;
		}
		lengths[0] = 0;

		// Create code length table
		FrInflateTableEntry* code_length_table;
		if(frBuildInflateTable(FR_CODE_LENGTH_TABLE_BIT_LENGTH, code_length_length, 19, lengths, max_code_length, &code_length_table) != FR_SUCCESS)
		{
			return FR_ERROR_OUT_OF_MEMORY;
		}

		uint8_t literal_length_length[286] = {0};
		uint16_t literal_length_lengths[16] = {0};
		uint8_t distance_length[32] = {0};
		uint16_t distance_lengths[16] = {0};

		uint16_t code, symbol;
		for(uint16_t i = 0; i < literal_length_count + distance_count; ++i)
		{
			if(frReadFromTable(&pData->iterator, code_length_table, FR_CODE_LENGTH_TABLE_BIT_LENGTH, max_code_length, &symbol) != FR_SUCCESS)
			{
				frFreeInflateTable(code_length_table, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
				return FR_ERROR_CORRUPTED_FILE;
			}

			switch(symbol)
			{
			case 16:
				if(frLSBFBits(&pData->iterator, 2, &code) != FR_SUCCESS)
				{
					frFreeInflateTable(code_length_table, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
					return FR_ERROR_CORRUPTED_FILE;
				}
				for(uint16_t j = i; j < i + code + 3; ++j)
				{
					if(j < literal_length_count)
					{
						literal_length_length[j] = literal_length_length[i - 1];
						++literal_length_lengths[literal_length_length[i - 1]];
						continue;
					}

					if(i <= literal_length_count)
					{
						distance_length[j - literal_length_count] = literal_length_length[i - 1];
						++distance_lengths[literal_length_length[i - 1]];
						continue;
					}

					distance_length[j - literal_length_count] = distance_length[i - literal_length_count - 1];
					++distance_lengths[distance_length[i - literal_length_count - 1]];
				}
				i += code + 2;
				break;

			case 17:
				if(frLSBFBits(&pData->iterator, 3, &code) != FR_SUCCESS)
				{
					frFreeInflateTable(code_length_table, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
					return FR_ERROR_CORRUPTED_FILE;
				}
				i += code + 2;
				break;

			case 18:
				if(frLSBFBits(&pData->iterator, 7, &code) != FR_SUCCESS)
				{
					frFreeInflateTable(code_length_table, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
					return FR_ERROR_CORRUPTED_FILE;
				}
				i += code + 10;
				break;

			default:
				if(i < literal_length_count)
				{
					literal_length_length[i] = symbol;
					++literal_length_lengths[symbol];
					if (symbol > literal_length_max_length) literal_length_max_length = symbol;
					break;
				}

				distance_length[i - literal_length_count] = symbol;
				++distance_lengths[symbol];
				if (symbol > distance_max_length) distance_max_length = symbol;
			}
		}
		literal_length_lengths[0] = 0;
		distance_lengths[0] = 0;

		// Free code length table
		frFreeInflateTable(code_length_table, FR_CODE_LENGTH_TABLE_BIT_LENGTH);

		// Create literal/length table
		if(frBuildInflateTable(FR_LITERAL_LENGTH_TABLE_BIT_LENGTH, literal_length_length, 286, literal_length_lengths, literal_length_max_length, &literal_length_table) != FR_SUCCESS)
		{
			return FR_ERROR_OUT_OF_MEMORY;
		}

		// Create distance table
		if(frBuildInflateTable(FR_DISTANCE_TABLE_BIT_LENGTH, distance_length, 32, distance_lengths, distance_max_length, &distance_table) != FR_SUCCESS)
		{
			frFreeInflateTable(literal_length_table, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			return FR_ERROR_OUT_OF_MEMORY;
		}
	}

	// Read deflated data
	uint16_t literal_length_symbol, distance_symbol, code;
	lldiv_t division;
	do
	{
		// Read literal/length symbol
		if(frReadFromTable(&pData->iterator, literal_length_table, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH, literal_length_max_length, &literal_length_symbol) != FR_SUCCESS)
		{
			frFreeInflateTable(literal_length_table, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(distance_table, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// End of block
		if (literal_length_symbol == 256) break;

		// Literal symbol
		if(literal_length_symbol < 256)
		{
			// Copy literal in output stream
			pData->result[pData->result_iterator] = literal_length_symbol;
			++pData->result_iterator;

			continue;
		}

		// Length symbol
		literal_length_symbol -= 257;

		// Read length extra bits
		if(frLSBFBits(&pData->iterator, literal_length_extra_bits[literal_length_symbol], &code) != FR_SUCCESS)
		{
			frFreeInflateTable(literal_length_table, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(distance_table, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Compute final length
		literal_length_symbol = literal_length_symbol + literal_length_offset[literal_length_symbol] + code;

		// Read distance symbol
		if(frReadFromTable(&pData->iterator, distance_table, FR_DISTANCE_TABLE_BIT_LENGTH, distance_max_length, &distance_symbol) != FR_SUCCESS)
		{
			frFreeInflateTable(literal_length_table, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(distance_table, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Read distance extra bits
		if(frLSBFBits(&pData->iterator, distance_extra_bits[distance_symbol], &code) != FR_SUCCESS)
		{
			frFreeInflateTable(literal_length_table, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(distance_table, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Compute final distance
		distance_symbol = distance_symbol + distance_offset[distance_symbol] + code;

		// Copy referenced block in output stream
		// Take care of special case: if length > distance
		division = lldiv(literal_length_symbol, distance_symbol);

		while(division.quot--)
		{
			memcpy(pData->result + pData->result_iterator, pData->result + pData->result_iterator - distance_symbol, distance_symbol);
			pData->result_iterator += distance_symbol;
		}

		memcpy(pData->result + pData->result_iterator, pData->result + pData->result_iterator - distance_symbol, division.rem);
		pData->result_iterator += division.rem;

	} while(literal_length_symbol != 256);

	// Free literal/length and distance tables
	frFreeInflateTable(literal_length_table, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
	frFreeInflateTable(distance_table, FR_DISTANCE_TABLE_BIT_LENGTH);

	return FR_SUCCESS;
}

/*
 * Inflate deflate-encoded data
 * - pData is the deflate-encoded input data
 * - size is the number of bytes in the input data
 * - ppResult is the buffer in which to store the result
 */
FrResult frInflate(const uint8_t* pData, size_t size, uint8_t** ppResult)
{
	// Build the inflate data
	FrInflateData data = {
		.iterator = {
			.data = pData,
			.size = size
		},
		.result = malloc(0xFFFFFF)
	};
	if(!data.result)
	{
		return FR_ERROR_OUT_OF_MEMORY;
	}

	do
	{
		// Inflate the next block
		FrResult result;
		if((result = frInflateBlock(&data)) != FR_SUCCESS)
		{
			return result;
		}
	} while(!data.final);

	// Data may not finish on a byte boundary
	frFinishByte(&data.iterator);

	// Make sure all input data was read
	if(data.iterator.size) return FR_ERROR_CORRUPTED_FILE;

	// Store the result
	*ppResult = data.result;

	return FR_SUCCESS;
}

// TODO:
// - do more checks on some read values / arguments
// - refactor / factorize : put ranges creation in frBuildInflateTable...
// - do the fixed huffman code part
// - comment more
// - improve consistency
// - remove logs
// - check spaces / tabs consistency
