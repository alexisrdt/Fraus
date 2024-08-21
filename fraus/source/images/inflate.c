#include "../../include/fraus/images/inflate.h"

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
 * - pData: input data stream
 * - size: remaining bytes to be read
 * - iterator: iterator over the bits of the current byte
 */
typedef struct FrInflateIterator
{
	const uint8_t* pData;
	size_t size;
	uint8_t iterator;
} FrInflateIterator;

/*
 * Structure to pass all data useful for inflate algorithm
 * - iterator: iterator over input data
 * - final: flag set if the deflate block is the last in the input data stream
 * - pResult: buffer to store the inflated result
 * - resultIterator: number of bytes written to result
 */
typedef struct FrInflateData
{
	FrInflateIterator iterator;
	uint8_t final;
	uint8_t* pResult;
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
 * - pSubTable: pointer to the sub table if the code is a prefix for longer codes
 * - symbol: symbol associated with the code if the code is not a prefix for longer codes
 * - length: length of the entry (0 = unused --> error if read, > table length (= maxLength) = sub table, otherwise = symbol)
 */
typedef struct FrInflateTableEntry
{
	union
	{
		FrInflateSubTableEntry* pSubTable;
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
 * - pIterator: iterator over the deflate encoded input data
 */
static uint8_t frNextBit(FrInflateIterator* pIterator)
{
	// If the input has already been entirely read, the last code may still need some bits
	// to match the table bit length, therefore return 0
	if(!pIterator->size) return 0;

	// Store the value of the bit
	uint8_t bit = ((*pIterator->pData) & (1 << pIterator->iterator)) >> pIterator->iterator;

	// Move the byte iterator by one place
	++pIterator->iterator;

	// If the byte is finished, start a new one
	if(pIterator->iterator == 8)
	{
		pIterator->iterator = 0;
		++pIterator->pData;
		--pIterator->size;
	}

	return bit;
}

/*
 * Finish the input byte being read
 * - pIterator: iterator over the deflate encoded input data
 */
static void frFinishByte(FrInflateIterator* pIterator)
{
	// If the iterator is already pointing to the first bit of a byte, there is nothing to do
	// Otherwise, start the next byte
	if(pIterator->iterator)
	{
		pIterator->iterator = 0;
		++pIterator->pData;
		--pIterator->size;
	}
}

/*
 * Read the next input bits as a uint16_t in an LSBF manner (Least Significant Bit First)
 * - pIterator: iterator over the deflate encoded input data
 * - count: number of bits to read
 * - pResult: output in which the value will be stored
 */
static FrResult frLSBFBits(FrInflateIterator* pIterator, uint8_t count, uint16_t* pResult)
{
	// Check that input has not already been entirely read
	if(!pIterator->size) return FR_ERROR_CORRUPTED_FILE;

	// Clear the output
	*pResult = 0;

	for(uint8_t bitPlace = 0; bitPlace < count; ++bitPlace)
	{
		// Store the next bit in the bitPlace-th place of the result
		*pResult |= (uint16_t)frNextBit(pIterator) << bitPlace;
	}

	return FR_SUCCESS;
}

/*
 * Read the next input bits as a uint16_t in an MSBF manner (Most Significant Bit First)
 * - pIterator: iterator over the deflate encoded input data
 * - count: number of bits to read
 * - pResult: output in which the value will be stored
 */
static FrResult frMSBFBits(FrInflateIterator* pIterator, uint8_t count, uint16_t* pResult)
{
	// Check that input has not already been entirely read
	if(!pIterator->size) return FR_ERROR_CORRUPTED_FILE;

	// Clear the output
	*pResult = 0;

	while(count--)
	{
		// Append the next bit at the end of the result
		*pResult = (*pResult << 1) | frNextBit(pIterator);
	}

	return FR_SUCCESS;
}

/*
 * Return bits to the input stream
 * - pIterator: iterator over the deflate encoded input data
 * - count: number of bits to return
 */
static void frReturnBits(FrInflateIterator* pIterator, uint8_t count)
{
	// Count how many whole bytes are to be returned
	const div_t byteRatio = div(count, 8);
	const uint8_t overflow = byteRatio.rem > pIterator->iterator ? 1 : 0;

	// Return the bits
	pIterator->pData -= byteRatio.quot + overflow;
	pIterator->size += byteRatio.quot + overflow;
	pIterator->iterator += (uint8_t)(8 * overflow - byteRatio.rem);
}

/*
 * Build an inflate lookup table
 * - tableLength: length of table indexes
 * - pSymbolLength: array of length of the code associated to each symbol
 * - symbolCount: number of symbols
 * - pLengths: array holding the count of occurences of each length
 * - maxLength: maximum bit length for this table
 * - ppTable: output in which the table will be stored
 */
static FrResult frBuildInflateTable(uint8_t tableLength, const uint8_t* pSymbolLength, uint16_t symbolCount, const uint16_t* pLengths, uint8_t maxLength, FrInflateTableEntry** ppTable)
{
	// Genreate ranges
	FrInflateRange* pRanges = malloc(symbolCount * sizeof(FrInflateRange));
	if(!pRanges) return FR_ERROR_OUT_OF_HOST_MEMORY;
	pRanges[0] = (FrInflateRange){.lastSymbol = 0, .length = pSymbolLength[0]};

	uint16_t rangeCount = 0;
	for(uint16_t symbol = 1; symbol < symbolCount; ++symbol)
	{
		if(pSymbolLength[symbol] != pSymbolLength[symbol - 1])
		{
			++rangeCount;
			pRanges[rangeCount].length = pSymbolLength[symbol];
		}

		pRanges[rangeCount].lastSymbol = symbol;
	}
	++rangeCount;

	// Generate first code for each length
	uint16_t* pNextCodes = malloc((maxLength + 1) * sizeof(uint16_t));
	if(!pNextCodes)
	{
		free(pRanges);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	pNextCodes[0] = 0;

	for(uint8_t length = 1; length <= maxLength; ++length)
	{
		pNextCodes[length] = (pNextCodes[length - 1] + pLengths[length - 1]) << 1;
	}

	// Allocate table
	const uint16_t subTableLength = maxLength - tableLength;

	const uint16_t tableSize = UINT16_C(1) << tableLength;
	const uint16_t subTableSize = UINT16_C(1) << subTableLength;

	FrInflateTableEntry* pTable = calloc(tableSize, sizeof(FrInflateTableEntry));
	if(!pTable)
	{
		free(pRanges);
		free(pNextCodes);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	// Build table entries
	uint16_t activeRange = 0;
	for(uint16_t symbol = 0; symbol <= pRanges[rangeCount - 1].lastSymbol; ++symbol)
	{
		// Update active range
		if(symbol > pRanges[activeRange].lastSymbol) ++activeRange;
		while(!pRanges[activeRange].length)
		{
			symbol = pRanges[activeRange].lastSymbol + 1;
			++activeRange;

			if(symbol > pRanges[rangeCount - 1].lastSymbol) goto end;
		}
		
		// Get next code
		uint16_t code = pNextCodes[pRanges[activeRange].length]++;

		// If short code
		if(pRanges[activeRange].length <= tableLength)
		{
			// Assign entry to all duplicate codes
			const uint16_t duplicateLength = tableLength - pRanges[activeRange].length;
			code = code << duplicateLength;
			for(uint16_t entryCode = code; entryCode < code + (UINT16_C(1) << duplicateLength); ++entryCode)
			{
				pTable[entryCode] = (FrInflateTableEntry){.length = pRanges[activeRange].length, .symbol = symbol};
			}

			continue;
		}

		// If long code

		// Compute code prefix and postfix to index in the table and the sub table
		const uint16_t subLength = pRanges[activeRange].length - tableLength;
		const uint16_t duplicateLength = maxLength - pRanges[activeRange].length;

		const uint16_t prefix = code >> subLength;
		const uint16_t postfix = (code & ((UINT16_C(1) << subLength) - 1)) << duplicateLength;

		// Create the sub table if it has not yet been created
		if(!pTable[prefix].length)
		{
			pTable[prefix].pSubTable = calloc(subTableSize, sizeof(FrInflateSubTableEntry));
			if(!pTable[prefix].pSubTable)
			{
				// Free already allocated sub tables
				for(uint16_t entryCode = 0; entryCode < tableSize - 1; ++entryCode)
				{
					if(pTable[entryCode].length > tableLength) free(pTable[entryCode].pSubTable);
				}
				free(pTable);
				free(pRanges);
				free(pNextCodes);

				return FR_ERROR_OUT_OF_HOST_MEMORY;
			}
			pTable[prefix].length = maxLength;
		}
		// Assign sub entry to all duplicate codes
		for(uint16_t subEntryCode = postfix; subEntryCode < postfix + (UINT16_C(1) << duplicateLength); ++subEntryCode)
		{
			pTable[prefix].pSubTable[subEntryCode] = (FrInflateSubTableEntry){.length = (uint8_t)subLength, .symbol = symbol};
		}
	}
	end:

	// Free ranges and next codes
	free(pRanges);
	free(pNextCodes);

	// Store result
	*ppTable = pTable;

	return FR_SUCCESS;
}

/*
 * Free an inflate lookup table
 * - pTable: lookup table to free
 * - tableLength: length of table indexes
 */
static void frFreeInflateTable(FrInflateTableEntry* pTable, uint8_t tableLength)
{
	// Free all contiguous entries with a length greater than the table length
	for(int16_t entryCode = (INT16_C(1) << tableLength) - 1; entryCode >= 0; --entryCode)
	{
		// If a length equal or less than the table length is reached, all sub tables have been freed
		if(pTable[entryCode].length > 0 && pTable[entryCode].length <= tableLength) break;

		// If the length is greater than the table length, free the sub table
		if(pTable[entryCode].length > tableLength) free(pTable[entryCode].pSubTable);
	}

	// Free the table
	free(pTable);
}

/*
 * Read the next input code from the given lookup table
 * - pIterator: iterator over the deflate encoded input data
 * - pTable: lookup table to read from
 * - tableLength: length of the primary level codes in the lookup table
 * - maxLength: maximum bit length for the table
 * - pSymbol: output in which the symbol will be stored
 */
static FrResult frReadFromTable(FrInflateIterator* pIterator, const FrInflateTableEntry* pTable, uint8_t tableLength, uint8_t maxLength, uint16_t* pSymbol)
{
	// Get code
	uint16_t code;
	if(frMSBFBits(pIterator, tableLength, &code) != FR_SUCCESS) return FR_ERROR_CORRUPTED_FILE;

	// Check invalid code
	if(!pTable[code].length) return FR_ERROR_CORRUPTED_FILE;

	// If short code
	if(pTable[code].length <= tableLength)
	{
		// Set symbol and return extra bits
		*pSymbol = pTable[code].symbol;
		frReturnBits(pIterator, tableLength - pTable[code].length);

		return FR_SUCCESS;
	}

	// If long code
	// No need to check for sub table: if it is a long code, the sub table is guaranteed to exist
	uint16_t subCode;
	if(frMSBFBits(pIterator, maxLength - tableLength, &subCode) != FR_SUCCESS) return FR_ERROR_CORRUPTED_FILE;

	// Check invalid sub code
	if(!pTable[code].pSubTable[subCode].length) return FR_ERROR_CORRUPTED_FILE;

	// Set symbol and return extra bits
	*pSymbol = pTable[code].pSubTable[subCode].symbol;
	frReturnBits(pIterator, maxLength - tableLength - pTable[code].pSubTable[subCode].length);

	return FR_SUCCESS;
}

/*
 * Inflate a deflate encoded block
 * - pData: inflate algorithm data
 */
static FrResult frInflateBlock(FrInflateData* pData)
{
	// Read the final block flag
	if(pData->iterator.size == 0) return FR_ERROR_CORRUPTED_FILE;
	pData->final = frNextBit(&pData->iterator);

	// Read the block type
	uint16_t blockType;
	if(frLSBFBits(&pData->iterator, 2, &blockType) != FR_SUCCESS) return FR_ERROR_CORRUPTED_FILE;

	// Invalid block type
	if(blockType == 3) return FR_ERROR_CORRUPTED_FILE;

	// Raw data block
	if(blockType == 0)
	{
		// Finish the current byte
		frFinishByte(&pData->iterator);

		// Make sure the is enough data to read (4 bytes for LEN and NLEN)
		if(pData->iterator.size < 4) return FR_ERROR_CORRUPTED_FILE;

		// Read LEN
		uint16_t length = FR_LSBF_TO_U16(pData->iterator.pData);

		// Make sure NLEN is the ones complement of LEN
		// Also make sure there is at least LEN bytes to read in input data
		if(length != ~FR_LSBF_TO_U16(pData->iterator.pData + 2) || length > pData->iterator.size - 4) return FR_ERROR_CORRUPTED_FILE;

		// Copy LEN bytes in the output buffer
		memcpy(pData->pResult + pData->resultIterator, pData->iterator.pData + 4, length);
		pData->resultIterator += length;

		// Update the input data iterator
		pData->iterator.pData += length + 4;
		pData->iterator.size -= length + 4;

		return FR_SUCCESS;
	}

	// Encoded block
	FrInflateTableEntry* pLiteralLengthTable;
	uint8_t pLiteralLengthSymbolLength[288] = {0};
	uint16_t pLiteralLengthLengthCount[16] = {0};
	uint8_t literalLengthMaxLength = 0;
	const uint8_t pLiteralLengthExtraBits[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
	const uint16_t pLiteralLengthOffset[29] = {3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 7, 10, 13, 16, 19, 26, 33, 40, 47, 62, 77, 92, 107, 138, 169, 200, 230};

	FrInflateTableEntry* pDistanceTable;
	uint8_t pDistanceSymbolLength[32] = {0};
	uint16_t pDistanceLengthCount[16] = {0};
	uint8_t distanceMaxLength = 0;
	const uint8_t pDistanceExtraBits[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
	const uint16_t pDistanceOffset[30] = {1, 1, 1, 1, 1, 2, 3, 6, 9, 16, 23, 38, 53, 84, 115, 178, 241, 368, 495, 750, 1005, 1516, 2027, 3050, 4073, 6120, 8167, 12262, 16357, 24548};

	// Fixed Huffman encoding
	if(blockType == 1)
	{
		// Create literal/length data
		for(uint16_t symbol = 0; symbol <= 143; ++symbol)
		{
			pLiteralLengthSymbolLength[symbol] = 8;
		}
		for(uint16_t symbol = 144; symbol <= 255; ++symbol)
		{
			pLiteralLengthSymbolLength[symbol] = 9;
		}
		for(uint16_t symbol = 256; symbol <= 279; ++symbol)
		{
			pLiteralLengthSymbolLength[symbol] = 7;
		}
		for(uint16_t symbol = 280; symbol <= 287; ++symbol)
		{
			pLiteralLengthSymbolLength[symbol] = 8;
		}
		pLiteralLengthLengthCount[7] = 24;
		pLiteralLengthLengthCount[8] = 152;
		pLiteralLengthLengthCount[9] = 112;
		literalLengthMaxLength = 9;

		// Create distance data
		for(uint16_t symbol = 0; symbol < 32; ++symbol)
		{
			pDistanceSymbolLength[symbol] = 5;
		}
		pDistanceLengthCount[5] = 32;
		distanceMaxLength = 5;
	}

	// Dynamic Huffman encoding
	else
	{
		// Read count of literal/length code lengths
		uint16_t literalLengthCount;
		if(frLSBFBits(&pData->iterator, 5, &literalLengthCount) != FR_SUCCESS || literalLengthCount > 29) return FR_ERROR_CORRUPTED_FILE;
		literalLengthCount += 257;

		// Read count of distance code lengths
		uint16_t distanceCount;
		if(frLSBFBits(&pData->iterator, 5, &distanceCount) != FR_SUCCESS || distanceCount > 31) return FR_ERROR_CORRUPTED_FILE;
		distanceCount += 1;

		// Read count of code length code lengths
		uint16_t codeLengthCount;
		if(frLSBFBits(&pData->iterator, 4, &codeLengthCount) != FR_SUCCESS || codeLengthCount > 15) return FR_ERROR_CORRUPTED_FILE;
		codeLengthCount += 4;

		// Define the order of code length symbols
		const uint16_t pCodeLengthSymbolOrder[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

		// Read and count lengths, find max length
		uint8_t pCodeLengthSymbolLength[19] = {0};
		uint16_t pCodeLengthLengthCount[8] = {0};
		uint8_t codeLengthMaxLength = 0;
		uint16_t length;
		for(uint16_t symbolIndex = 0; symbolIndex < codeLengthCount; ++symbolIndex)
		{
			// Read 3 bit long length
			if(frLSBFBits(&pData->iterator, 3, &length) != FR_SUCCESS) return FR_ERROR_CORRUPTED_FILE;

			// Update code length data
			pCodeLengthSymbolLength[pCodeLengthSymbolOrder[symbolIndex]] = (uint8_t)length;
			++pCodeLengthLengthCount[length];
			if(length > codeLengthMaxLength) codeLengthMaxLength = (uint8_t)length;
		}
		// Reset count of 0 length codes to 0 for first codes creation
		pCodeLengthLengthCount[0] = 0;

		// Create code length table
		FrInflateTableEntry* pCodeLengthTable;
		if(frBuildInflateTable(FR_CODE_LENGTH_TABLE_BIT_LENGTH, pCodeLengthSymbolLength, 19, pCodeLengthLengthCount, codeLengthMaxLength, &pCodeLengthTable) != FR_SUCCESS)
		{
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}

		// Read literal/length and distance codes lengths
		uint16_t symbol, extraBits;
		for(uint16_t symbolIndex = 0; symbolIndex < literalLengthCount + distanceCount; ++symbolIndex)
		{
			// Read next code length code
			if(frReadFromTable(&pData->iterator, pCodeLengthTable, FR_CODE_LENGTH_TABLE_BIT_LENGTH, codeLengthMaxLength, &symbol) != FR_SUCCESS)
			{
				frFreeInflateTable(pCodeLengthTable, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
				return FR_ERROR_CORRUPTED_FILE;
			}

			switch(symbol)
			{
				// Copy the previous code length
				case 16:
					if(frLSBFBits(&pData->iterator, 2, &extraBits) != FR_SUCCESS)
					{
						frFreeInflateTable(pCodeLengthTable, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
						return FR_ERROR_CORRUPTED_FILE;
					}
					for(uint16_t copyIndex = symbolIndex; copyIndex < symbolIndex + extraBits + 3; ++copyIndex)
					{
						// Literal/length index
						if(copyIndex < literalLengthCount)
						{
							pLiteralLengthSymbolLength[copyIndex] = pLiteralLengthSymbolLength[symbolIndex - 1];
							++pLiteralLengthLengthCount[pLiteralLengthSymbolLength[symbolIndex - 1]];
							continue;
						}

						// Distance index with literal/length reference
						if(symbolIndex <= literalLengthCount)
						{
							pDistanceSymbolLength[copyIndex - literalLengthCount] = pLiteralLengthSymbolLength[symbolIndex - 1];
							++pDistanceLengthCount[pLiteralLengthSymbolLength[symbolIndex - 1]];
							continue;
						}

						// Distance index with distance reference
						pDistanceSymbolLength[copyIndex - literalLengthCount] = pDistanceSymbolLength[symbolIndex - literalLengthCount - 1];
						++pDistanceLengthCount[pDistanceSymbolLength[symbolIndex - literalLengthCount - 1]];
					}
					symbolIndex += extraBits + 2;
					break;

				// Copy 0 (skip indexes)
				case 17:
					if(frLSBFBits(&pData->iterator, 3, &extraBits) != FR_SUCCESS)
					{
						frFreeInflateTable(pCodeLengthTable, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
						return FR_ERROR_CORRUPTED_FILE;
					}
					symbolIndex += extraBits + 2;
					break;

				// Copy 0 (skip indexes)
				case 18:
					if(frLSBFBits(&pData->iterator, 7, &extraBits) != FR_SUCCESS)
					{
						frFreeInflateTable(pCodeLengthTable, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
						return FR_ERROR_CORRUPTED_FILE;
					}
					symbolIndex += extraBits + 10;
					break;

				// Actual symbol
				default:
					// Literal/length symbol
					if(symbolIndex < literalLengthCount)
					{
						pLiteralLengthSymbolLength[symbolIndex] = (uint8_t)symbol;
						++pLiteralLengthLengthCount[symbol];
						if(symbol > literalLengthMaxLength) literalLengthMaxLength = (uint8_t)symbol;
						break;
					}

					// Distance symbol
					pDistanceSymbolLength[symbolIndex - literalLengthCount] = (uint8_t)symbol;
					++pDistanceLengthCount[symbol];
					if(symbol > distanceMaxLength) distanceMaxLength = (uint8_t)symbol;
			}
		}
		// Reset count of 0 length codes to 0 for first codes creation
		pLiteralLengthLengthCount[0] = 0;
		pDistanceLengthCount[0] = 0;

		// Free code length table
		frFreeInflateTable(pCodeLengthTable, FR_CODE_LENGTH_TABLE_BIT_LENGTH);
	}

	// Create literal/length table
	if(frBuildInflateTable(FR_LITERAL_LENGTH_TABLE_BIT_LENGTH, pLiteralLengthSymbolLength, 286, pLiteralLengthLengthCount, literalLengthMaxLength, &pLiteralLengthTable) != FR_SUCCESS)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	// Create distance table
	if(frBuildInflateTable(FR_DISTANCE_TABLE_BIT_LENGTH, pDistanceSymbolLength, 32, pDistanceLengthCount, distanceMaxLength, &pDistanceTable) != FR_SUCCESS)
	{
		frFreeInflateTable(pLiteralLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	// Read deflated data
	uint16_t literalLengthSymbol, distanceSymbol, extraBits;
	lldiv_t lengthDistanceRatio;
	do
	{
		// Read literal/length symbol
		if(frReadFromTable(&pData->iterator, pLiteralLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH, literalLengthMaxLength, &literalLengthSymbol) != FR_SUCCESS)
		{
			frFreeInflateTable(pLiteralLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(pDistanceTable, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Invalid value
		if(literalLengthSymbol >= 286)
		{
			frFreeInflateTable(pLiteralLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(pDistanceTable, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// End of block
		if(literalLengthSymbol == 256) break;

		// Literal symbol
		if(literalLengthSymbol < 256)
		{
			// Copy literal in output stream
			pData->pResult[pData->resultIterator] = (uint8_t)literalLengthSymbol;
			++pData->resultIterator;

			continue;
		}

		// Length symbol
		literalLengthSymbol -= 257;

		// Read length extra bits
		if(frLSBFBits(&pData->iterator, pLiteralLengthExtraBits[literalLengthSymbol], &extraBits) != FR_SUCCESS)
		{
			frFreeInflateTable(pLiteralLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(pDistanceTable, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Compute final length
		literalLengthSymbol += pLiteralLengthOffset[literalLengthSymbol] + extraBits;

		// Read distance symbol
		if(frReadFromTable(&pData->iterator, pDistanceTable, FR_DISTANCE_TABLE_BIT_LENGTH, distanceMaxLength, &distanceSymbol) != FR_SUCCESS)
		{
			frFreeInflateTable(pLiteralLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(pDistanceTable, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Read distance extra bits
		if(frLSBFBits(&pData->iterator, pDistanceExtraBits[distanceSymbol], &extraBits) != FR_SUCCESS)
		{
			frFreeInflateTable(pLiteralLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
			frFreeInflateTable(pDistanceTable, FR_DISTANCE_TABLE_BIT_LENGTH);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Compute final distance
		distanceSymbol += pDistanceOffset[distanceSymbol] + extraBits;

		// Copy referenced block in output stream
		// Take care of special case: if length > distance
		lengthDistanceRatio = lldiv(literalLengthSymbol, distanceSymbol);

		while(lengthDistanceRatio.quot--)
		{
			memcpy(pData->pResult + pData->resultIterator, pData->pResult + pData->resultIterator - distanceSymbol, distanceSymbol);
			pData->resultIterator += distanceSymbol;
		}

		memcpy(pData->pResult + pData->resultIterator, pData->pResult + pData->resultIterator - distanceSymbol, lengthDistanceRatio.rem);
		pData->resultIterator += lengthDistanceRatio.rem;

	} while(true);

	// Free literal/length and distance tables
	frFreeInflateTable(pLiteralLengthTable, FR_LITERAL_LENGTH_TABLE_BIT_LENGTH);
	frFreeInflateTable(pDistanceTable, FR_DISTANCE_TABLE_BIT_LENGTH);

	return FR_SUCCESS;
}

/*
 * Inflate deflate encoded data
 * - pData: deflate encoded input data
 * - size: number of bytes in the input data
 * - ppResult: buffer in which to store the result
 */
FrResult frInflate(const uint8_t* pData, size_t size, uint8_t* pResult)
{
	// Build the inflate data
	FrInflateData data = {
		.iterator = {
			.pData = pData,
			.size = size
		},
		.pResult = pResult
	};

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

	return FR_SUCCESS;
}
