#include "../../include/fraus/images/inflate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// LSBF = Least Significant Byte First
// Transforms two bytes in LSBF order into a uint16_t
#define FR_LSBF_TO_U16(bytes) \
((uint16_t)((bytes)[0]) | ((uint16_t)((bytes)[1]) << 8))

/*
 * Iterator over deflate encoded data
 * - data: input data stream
 * - size: remaining bytes to be read
 * - iterator: iterator over the bits of the current byte
 */
typedef struct FrInflateIterator
{
	const uint8_t* data;
	size_t size;
	uint8_t iterator;
} FrInflateIterator;

/*
 * Structure to pass all data useful for inflate algorithm
 * - iterator: iterator over input data
 * - final: flag set if the deflate block is the last in the input data stream
 * - result: buffer to store the inflated result
 * - resultIterator: number of bytes written to result
 */
typedef struct FrInflateData
{
	FrInflateIterator iterator;
	uint8_t final;
	uint8_t* result;
	size_t resultIterator;
} FrInflateData;

/*
 * Range of contiguous symbols that have the same length
 * - lastSymbol: last symbol in the range
 * - length: length of associated codes
 */
typedef struct FrInflateRange
{
	uint16_t lastSymbol;
	uint8_t length;
} FrInflateRange;

/*
 * Entry in a sub table of a lookup table
 * - symbol: symbol associated with the code
 * - length: length of the sub entry (0 = unused --> error if read, otherwise = symbol)
 */
typedef struct FrInflateSubTableEntry
{
	uint16_t symbol;
	uint8_t length;
} FrInflateSubTableEntry;

/*
 * Entry in the primary level lookup table
 * - subTable: pointer to the sub table if the code is a prefix for longer codes
 * - symbol: symbol associated with the code if the code is not a prefix for longer codes
 * - length: length of the entry (0 = unused --> error if read, > table length (= maxLength) = sub table, otherwise = symbol)
 */
typedef struct FrInflateTableEntry
{
	union
	{
		FrInflateSubTableEntry* subTable;
		uint16_t symbol;
	};
	uint8_t length;
} FrInflateTableEntry;

// Bit lengths for primary level codes of lookup tables
#define FR_CODE_LENGTH_TABLE_BIT_LENGTH    5
#define FR_LITERAL_LENGTH_TABLE_BIT_LENGTH 9
#define FR_DISTANCE_TABLE_BIT_LENGTH       6

/*
 * Read the next bit in the input data stream if available
 * - iterator: iterator over the deflate encoded input data
 */
static uint8_t frNextBit(FrInflateIterator* const iterator)
{
	// If the input has already been entirely read, the last code may still need some bits
	// to match the table bit length, therefore return 0
	if(!iterator->size)
	{
		return 0;
	}

	// Store the value of the bit
	uint8_t bit = ((*iterator->data) & (1 << iterator->iterator)) >> iterator->iterator;

	// Move the byte iterator by one place
	++iterator->iterator;

	// If the byte is finished, start a new one
	if(iterator->iterator == 8)
	{
		iterator->iterator = 0;
		++iterator->data;
		--iterator->size;
	}

	return bit;
}

/*
 * Finish the input byte being read
 * - iterator: iterator over the deflate encoded input data
 */
static void frFinishByte(FrInflateIterator* const iterator)
{
	// If the iterator is already pointing to the first bit of a byte, there is nothing to do
	// Otherwise, start the next byte
	if(iterator->iterator)
	{
		iterator->iterator = 0;
		++iterator->data;
		--iterator->size;
	}
}

/*
 * Read the next input bits as a uint16_t in an LSBF manner (Least Significant Bit First)
 * - iterator: iterator over the deflate encoded input data
 * - count: number of bits to read
 * - result: output in which the value will be stored
 */
static FrResult frLSBFBits(FrInflateIterator* const iterator, const uint8_t count, uint16_t* const result)
{
	// Check that input has not already been entirely read
	if(!iterator->size)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Clear the output
	*result = 0;

	for(uint8_t bitPlace = 0; bitPlace < count; ++bitPlace)
	{
		// Store the next bit in the bitPlace-th place of the result
		*result |= (uint16_t)frNextBit(iterator) << bitPlace;
	}

	return FR_SUCCESS;
}

/*
 * Read the next input bits as a uint16_t in an MSBF manner (Most Significant Bit First)
 * - iterator: iterator over the deflate encoded input data
 * - count: number of bits to read
 * - result: output in which the value will be stored
 */
static FrResult frMSBFBits(FrInflateIterator* const iterator, uint8_t count, uint16_t* const result)
{
	// Check that input has not already been entirely read
	if(!iterator->size)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Clear the output
	*result = 0;

	while(count--)
	{
		// Append the next bit at the end of the result
		*result = (*result << 1) | frNextBit(iterator);
	}

	return FR_SUCCESS;
}

/*
 * Return bits to the input stream
 * - iterator: iterator over the deflate encoded input data
 * - count: number of bits to return
 */
static void frReturnBits(FrInflateIterator* const iterator, const uint8_t count)
{
	// Count how many whole bytes are to be returned
	const div_t byteRatio = div(count, 8);
	const uint8_t overflow = byteRatio.rem > iterator->iterator ? 1 : 0;

	// Return the bits
	iterator->data -= byteRatio.quot + overflow;
	iterator->size += byteRatio.quot + overflow;
	iterator->iterator += (uint8_t)(8 * overflow - byteRatio.rem);
}

/*
 * Build an inflate lookup table
 * - tableLength: length of table indexes
 * - symbolLength: array of length of the code associated to each symbol
 * - symbolCount: number of symbols
 * - lengths: array holding the count of occurences of each length
 * - maxLength: maximum bit length for this table
 * - table: output in which the table will be stored
 */
static FrResult frBuildInflateTable(const uint8_t tableLength, const uint8_t* const symbolLength, const uint16_t symbolCount, const uint16_t* const lengths, const uint8_t maxLength, FrInflateTableEntry** const table)
{
	// Genreate ranges
	FrInflateRange* ranges = malloc(symbolCount * sizeof(ranges[0]));
	if(!ranges)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	ranges[0] = (FrInflateRange){.lastSymbol = 0, .length = symbolLength[0]};

	uint16_t rangeCount = 0;
	for(uint16_t symbol = 1; symbol < symbolCount; ++symbol)
	{
		if(symbolLength[symbol] != symbolLength[symbol - 1])
		{
			++rangeCount;
			ranges[rangeCount].length = symbolLength[symbol];
		}

		ranges[rangeCount].lastSymbol = symbol;
	}
	++rangeCount;

	// Generate first code for each length
	uint16_t* const nextCodes = malloc((maxLength + 1) * sizeof(nextCodes[0]));
	if(!nextCodes)
	{
		free(ranges);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	nextCodes[0] = 0;

	for(uint8_t length = 1; length <= maxLength; ++length)
	{
		nextCodes[length] = (nextCodes[length - 1] + lengths[length - 1]) << 1;
	}

	// Allocate table
	const uint16_t subTableLength = maxLength - tableLength;

	const uint16_t tableSize = UINT16_C(1) << tableLength;
	const uint16_t subTableSize = UINT16_C(1) << subTableLength;

	*table = calloc(tableSize, sizeof((*table)[0]));
	if(!*table)
	{
		free(ranges);
		free(nextCodes);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	// Build table entries
	uint16_t activeRange = 0;
	for(uint16_t symbol = 0; symbol <= ranges[rangeCount - 1].lastSymbol; ++symbol)
	{
		// Update active range
		if(symbol > ranges[activeRange].lastSymbol)
		{
			++activeRange;
		}
		while(!ranges[activeRange].length)
		{
			symbol = ranges[activeRange].lastSymbol + 1;
			++activeRange;

			if(symbol > ranges[rangeCount - 1].lastSymbol)
			{
				goto end;
			}
		}
		
		// Get next code
		uint16_t code = nextCodes[ranges[activeRange].length]++;

		// If short code
		if(ranges[activeRange].length <= tableLength)
		{
			// Assign entry to all duplicate codes
			const uint16_t duplicateLength = tableLength - ranges[activeRange].length;
			code = code << duplicateLength;
			for(uint16_t entryCode = code; entryCode < code + (UINT16_C(1) << duplicateLength); ++entryCode)
			{
				(*table)[entryCode] = (FrInflateTableEntry){.length = ranges[activeRange].length, .symbol = symbol};
			}

			continue;
		}

		// If long code

		// Compute code prefix and postfix to index in the table and the sub table
		const uint16_t subLength = ranges[activeRange].length - tableLength;
		const uint16_t duplicateLength = maxLength - ranges[activeRange].length;

		const uint16_t prefix = code >> subLength;
		const uint16_t postfix = (code & ((UINT16_C(1) << subLength) - 1)) << duplicateLength;

		// Create the sub table if it has not yet been created
		if(!(*table)[prefix].length)
		{
			(*table)[prefix].subTable = calloc(subTableSize, sizeof(FrInflateSubTableEntry));
			if(!(*table)[prefix].subTable)
			{
				// Free already allocated sub tables
				for(uint16_t entryCode = 0; entryCode < tableSize - 1; ++entryCode)
				{
					if((*table)[entryCode].length > tableLength)
					{
						free((*table)[entryCode].subTable);
					}
				}
				free(*table);
				free(ranges);
				free(nextCodes);

				return FR_ERROR_OUT_OF_HOST_MEMORY;
			}
			(*table)[prefix].length = maxLength;
		}
		// Assign sub entry to all duplicate codes
		for(uint16_t subEntryCode = postfix; subEntryCode < postfix + (UINT16_C(1) << duplicateLength); ++subEntryCode)
		{
			(*table)[prefix].subTable[subEntryCode] = (FrInflateSubTableEntry){.length = (uint8_t)subLength, .symbol = symbol};
		}
	}
	end:

	// Free ranges and next codes
	free(ranges);
	free(nextCodes);

	return FR_SUCCESS;
}

/*
 * Free an inflate lookup table
 * - table: lookup table to free
 * - tableLength: length of table indexes
 */
static void frFreeInflateTable(FrInflateTableEntry* const table, const uint8_t tableLength)
{
	// Free all contiguous entries with a length greater than the table length
	for(int16_t entryCode = (INT16_C(1) << tableLength) - 1; entryCode >= 0; --entryCode)
	{
		// If a length equal or less than the table length is reached, all sub tables have been freed
		if(table[entryCode].length > 0 && table[entryCode].length <= tableLength)
		{
			break;
		}

		// If the length is greater than the table length, free the sub table
		if(table[entryCode].length > tableLength)
		{
			free(table[entryCode].subTable);
		}
	}

	// Free the table
	free(table);
}

/*
 * Read the next input code from the given lookup table
 * - iterator: iterator over the deflate encoded input data
 * - table: lookup table to read from
 * - tableLength: length of the primary level codes in the lookup table
 * - maxLength: maximum bit length for the table
 * - symbol: output in which the symbol will be stored
 */
static FrResult frReadFromTable(FrInflateIterator* const iterator, const FrInflateTableEntry* const table, const uint8_t tableLength, const uint8_t maxLength, uint16_t* const symbol)
{
	// Get code
	uint16_t code;
	if(frMSBFBits(iterator, tableLength, &code) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Check invalid code
	if(!table[code].length)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	// If short code
	if(table[code].length <= tableLength)
	{
		// Set symbol and return extra bits
		*symbol = table[code].symbol;
		frReturnBits(iterator, tableLength - table[code].length);

		return FR_SUCCESS;
	}

	// If long code
	// No need to check for sub table: if it is a long code, the sub table is guaranteed to exist
	uint16_t subCode;
	if(frMSBFBits(iterator, maxLength - tableLength, &subCode) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Check invalid sub code
	if(!table[code].subTable[subCode].length)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Set symbol and return extra bits
	*symbol = table[code].subTable[subCode].symbol;
	frReturnBits(iterator, maxLength - tableLength - table[code].subTable[subCode].length);

	return FR_SUCCESS;
}

/*
 * Inflate a deflate encoded block
 * - data: inflate algorithm data
 */
static FrResult frInflateBlock(FrInflateData* const data)
{
	// Read the final block flag
	if(data->iterator.size == 0)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	data->final = frNextBit(&data->iterator);

	// Read the block type
	uint16_t blockType;
	if(frLSBFBits(&data->iterator, 2, &blockType) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Invalid block type
	if(blockType == 3)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Raw data block
	if(blockType == 0)
	{
		// Finish the current byte
		frFinishByte(&data->iterator);

		// Make sure the is enough data to read (4 bytes for LEN and NLEN)
		if(data->iterator.size < 4)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Read LEN
		uint16_t length = FR_LSBF_TO_U16(data->iterator.data);

		// Make sure NLEN is the ones complement of LEN
		// Also make sure there is at least LEN bytes to read in input data
		if(length != ~FR_LSBF_TO_U16(data->iterator.data + 2) || length > data->iterator.size - 4)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Copy LEN bytes in the output buffer
		memcpy(data->result + data->resultIterator, data->iterator.data + 4, length);
		data->resultIterator += length;

		// Update the input data iterator
		data->iterator.data += length + 4;
		data->iterator.size -= length + 4;

		return FR_SUCCESS;
	}

	// Encoded block
	FrInflateTableEntry* literalLengthTable;
	uint8_t literalLengthSymbolLength[288] = {0};
	uint16_t literalLengthLengthCount[16] = {0};
	uint8_t literalLengthMaxLength = 0;
	const uint8_t literalLengthExtraBits[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
	const uint16_t literalLengthOffset[29] = {3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 7, 10, 13, 16, 19, 26, 33, 40, 47, 62, 77, 92, 107, 138, 169, 200, 230};

	FrInflateTableEntry* distanceTable;
	uint8_t distanceSymbolLength[32] = {0};
	uint16_t distanceLengthCount[16] = {0};
	uint8_t distanceMaxLength = 0;
	const uint8_t distanceExtraBits[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
	const uint16_t distanceOffset[30] = {1, 1, 1, 1, 1, 2, 3, 6, 9, 16, 23, 38, 53, 84, 115, 178, 241, 368, 495, 750, 1005, 1516, 2027, 3050, 4073, 6120, 8167, 12262, 16357, 24548};

	// Fixed Huffman encoding
	if(blockType == 1)
	{
		// Create literal/length data
		for(uint16_t symbol = 0; symbol <= 143; ++symbol)
		{
			literalLengthSymbolLength[symbol] = 8;
		}
		for(uint16_t symbol = 144; symbol <= 255; ++symbol)
		{
			literalLengthSymbolLength[symbol] = 9;
		}
		for(uint16_t symbol = 256; symbol <= 279; ++symbol)
		{
			literalLengthSymbolLength[symbol] = 7;
		}
		for(uint16_t symbol = 280; symbol <= 287; ++symbol)
		{
			literalLengthSymbolLength[symbol] = 8;
		}
		literalLengthLengthCount[7] = 24;
		literalLengthLengthCount[8] = 152;
		literalLengthLengthCount[9] = 112;
		literalLengthMaxLength = 9;

		// Create distance data
		for(uint16_t symbol = 0; symbol < 32; ++symbol)
		{
			distanceSymbolLength[symbol] = 5;
		}
		distanceLengthCount[5] = 32;
		distanceMaxLength = 5;
	}

	// Dynamic Huffman encoding
	else
	{
		// Read count of literal/length code lengths
		uint16_t literalLengthCount;
		if(frLSBFBits(&data->iterator, 5, &literalLengthCount) != FR_SUCCESS || literalLengthCount > 29)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		literalLengthCount += 257;

		// Read count of distance code lengths
		uint16_t distanceCount;
		if(frLSBFBits(&data->iterator, 5, &distanceCount) != FR_SUCCESS || distanceCount > 31)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		distanceCount += 1;

		// Read count of code length code lengths
		uint16_t codeLengthCount;
		if(frLSBFBits(&data->iterator, 4, &codeLengthCount) != FR_SUCCESS || codeLengthCount > 15)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		codeLengthCount += 4;

		// Define the order of code length symbols
		const uint16_t codeLengthSymbolOrder[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

		// Read and count lengths, find max length
		uint8_t codeLengthSymbolLength[19] = {0};
		uint16_t codeLengthLengthCount[8] = {0};
		uint8_t codeLengthMaxLength = 0;
		uint16_t length;
		for(uint16_t symbolIndex = 0; symbolIndex < codeLengthCount; ++symbolIndex)
		{
			// Read 3 bit long length
			if(frLSBFBits(&data->iterator, 3, &length) != FR_SUCCESS) return FR_ERROR_CORRUPTED_FILE;

			// Update code length data
			codeLengthSymbolLength[codeLengthSymbolOrder[symbolIndex]] = (uint8_t)length;
			++codeLengthLengthCount[length];
			if(length > codeLengthMaxLength)
			{
				codeLengthMaxLength = (uint8_t)length;
			}
		}
		// Reset count of 0 length codes to 0 for first codes creation
		codeLengthLengthCount[0] = 0;

		// Create code length table
		FrInflateTableEntry* codeLengthTable;
		if(frBuildInflateTable(FR_CODE_LENGTH_TABLE_BIT_LENGTH, codeLengthSymbolLength, 19, codeLengthLengthCount, codeLengthMaxLength, &codeLengthTable) != FR_SUCCESS)
		{
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}

		// Read literal/length and distance codes lengths
		uint16_t symbol, extraBits;
		for(uint16_t symbolIndex = 0; symbolIndex < literalLengthCount + distanceCount; ++symbolIndex)
		{
			// Read next code length code
			if(frReadFromTable(&data->iterator, codeLengthTable, FR_CODE_LENGTH_TABLE_BIT_LENGTH, codeLengthMaxLength, &symbol) != FR_SUCCESS)
			{
				frFreeInflateTable(codeLengthTable, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
				return FR_ERROR_CORRUPTED_FILE;
			}

			switch(symbol)
			{
				// Copy the previous code length
				case 16:
					if(frLSBFBits(&data->iterator, 2, &extraBits) != FR_SUCCESS)
					{
						frFreeInflateTable(codeLengthTable, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
						return FR_ERROR_CORRUPTED_FILE;
					}
					for(uint16_t copyIndex = symbolIndex; copyIndex < symbolIndex + extraBits + 3; ++copyIndex)
					{
						// Literal/length index
						if(copyIndex < literalLengthCount)
						{
							literalLengthSymbolLength[copyIndex] = literalLengthSymbolLength[symbolIndex - 1];
							++literalLengthLengthCount[literalLengthSymbolLength[symbolIndex - 1]];
							continue;
						}

						// Distance index with literal/length reference
						if(symbolIndex <= literalLengthCount)
						{
							distanceSymbolLength[copyIndex - literalLengthCount] = literalLengthSymbolLength[symbolIndex - 1];
							++distanceLengthCount[literalLengthSymbolLength[symbolIndex - 1]];
							continue;
						}

						// Distance index with distance reference
						distanceSymbolLength[copyIndex - literalLengthCount] = distanceSymbolLength[symbolIndex - literalLengthCount - 1];
						++distanceLengthCount[distanceSymbolLength[symbolIndex - literalLengthCount - 1]];
					}
					symbolIndex += extraBits + 2;
					break;

				// Copy 0 (skip indexes)
				case 17:
					if(frLSBFBits(&data->iterator, 3, &extraBits) != FR_SUCCESS)
					{
						frFreeInflateTable(codeLengthTable, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
						return FR_ERROR_CORRUPTED_FILE;
					}
					symbolIndex += extraBits + 2;
					break;

				// Copy 0 (skip indexes)
				case 18:
					if(frLSBFBits(&data->iterator, 7, &extraBits) != FR_SUCCESS)
					{
						frFreeInflateTable(codeLengthTable, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
						return FR_ERROR_CORRUPTED_FILE;
					}
					symbolIndex += extraBits + 10;
					break;

				// Actual symbol
				default:
					// Literal/length symbol
					if(symbolIndex < literalLengthCount)
					{
						literalLengthSymbolLength[symbolIndex] = (uint8_t)symbol;
						++literalLengthLengthCount[symbol];
						if(symbol > literalLengthMaxLength)
						{
							literalLengthMaxLength = (uint8_t)symbol;
						}
						break;
					}

					// Distance symbol
					distanceSymbolLength[symbolIndex - literalLengthCount] = (uint8_t)symbol;
					++distanceLengthCount[symbol];
					if(symbol > distanceMaxLength)
					{
						distanceMaxLength = (uint8_t)symbol;
					}
			}
		}
		// Reset count of 0 length codes to 0 for first codes creation
		literalLengthLengthCount[0] = 0;
		distanceLengthCount[0] = 0;

		// Free code length table
		frFreeInflateTable(codeLengthTable, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
	}

	// Create literal/length table
	if(frBuildInflateTable(FR_LITERAL_LENGTH_TABLE_BIT_LENGTH, literalLengthSymbolLength, 286, literalLengthLengthCount, literalLengthMaxLength, &literalLengthTable) != FR_SUCCESS)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	// Create distance table
	if(frBuildInflateTable(FR_DISTANCE_TABLE_BIT_LENGTH, distanceSymbolLength, 32, distanceLengthCount, distanceMaxLength, &distanceTable) != FR_SUCCESS)
	{
		frFreeInflateTable(literalLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	// Read deflated data
	uint16_t literalLengthSymbol, distanceSymbol, extraBits;
	lldiv_t lengthDistanceRatio;
	do
	{
		// Read literal/length symbol
		if(frReadFromTable(&data->iterator, literalLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH, literalLengthMaxLength, &literalLengthSymbol) != FR_SUCCESS)
		{
			frFreeInflateTable(literalLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(distanceTable, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Invalid value
		if(literalLengthSymbol >= 286)
		{
			frFreeInflateTable(literalLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(distanceTable, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// End of block
		if(literalLengthSymbol == 256)
		{
			break;
		}

		// Literal symbol
		if(literalLengthSymbol < 256)
		{
			// Copy literal in output stream
			data->result[data->resultIterator] = (uint8_t)literalLengthSymbol;
			++data->resultIterator;

			continue;
		}

		// Length symbol
		literalLengthSymbol -= 257;

		// Read length extra bits
		if(frLSBFBits(&data->iterator, literalLengthExtraBits[literalLengthSymbol], &extraBits) != FR_SUCCESS)
		{
			frFreeInflateTable(literalLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(distanceTable, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Compute final length
		literalLengthSymbol += literalLengthOffset[literalLengthSymbol] + extraBits;

		// Read distance symbol
		if(frReadFromTable(&data->iterator, distanceTable, FR_DISTANCE_TABLE_BIT_LENGTH, distanceMaxLength, &distanceSymbol) != FR_SUCCESS)
		{
			frFreeInflateTable(literalLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(distanceTable, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Read distance extra bits
		if(frLSBFBits(&data->iterator, distanceExtraBits[distanceSymbol], &extraBits) != FR_SUCCESS)
		{
			frFreeInflateTable(literalLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(distanceTable, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Compute final distance
		distanceSymbol += distanceOffset[distanceSymbol] + extraBits;

		// Copy referenced block in output stream
		// Take care of special case: if length > distance
		lengthDistanceRatio = lldiv(literalLengthSymbol, distanceSymbol);

		while(lengthDistanceRatio.quot--)
		{
			memcpy(data->result + data->resultIterator, data->result + data->resultIterator - distanceSymbol, distanceSymbol);
			data->resultIterator += distanceSymbol;
		}

		memcpy(data->result + data->resultIterator, data->result + data->resultIterator - distanceSymbol, lengthDistanceRatio.rem);
		data->resultIterator += lengthDistanceRatio.rem;

	} while(true);

	// Free literal/length and distance tables
	frFreeInflateTable(literalLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
	frFreeInflateTable(distanceTable, FR_DISTANCE_TABLE_BIT_LENGTH);

	return FR_SUCCESS;
}

/*
 * Inflate deflate encoded data
 * - data: deflate encoded input data
 * - size: number of bytes in the input data
 * - result: buffer in which to store the result
 */
FrResult frInflate(const uint8_t* const data, const size_t size, uint8_t* const result)
{
	// Build the inflate data
	FrInflateData inflateData = {
		.iterator = {
			.data = data,
			.size = size
		},
		.result = result
	};

	do
	{
		// Inflate the next block
		FrResult result;
		if((result = frInflateBlock(&inflateData)) != FR_SUCCESS)
		{
			return result;
		}
	} while(!inflateData.final);

	// Data may not finish on a byte boundary
	frFinishByte(&inflateData.iterator);

	// Make sure all input data was read
	if(inflateData.iterator.size)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	return FR_SUCCESS;
}
