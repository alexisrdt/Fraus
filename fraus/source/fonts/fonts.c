#include "fraus/fonts/fonts.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static FrResult frResize(void* pDataVoid, size_t newSize)
{
	if(!pDataVoid || newSize == 0)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	void** const pData = pDataVoid;
	void* const newData = realloc(*pData, newSize);
	if(!newData)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	*pData = newData;

	return FR_SUCCESS;
}

#define FR_CMAP_TAG FR_BYTES_TO_U32('c', 'm', 'a', 'p')
#define FR_GLYF_TAG FR_BYTES_TO_U32('g', 'l', 'y', 'f')
#define FR_HEAD_TAG FR_BYTES_TO_U32('h', 'e', 'a', 'd')
#define FR_HHEA_TAG FR_BYTES_TO_U32('h', 'h', 'e', 'a')
#define FR_HMTX_TAG FR_BYTES_TO_U32('h', 'm', 't', 'x')
#define FR_LOCA_TAG FR_BYTES_TO_U32('l', 'o', 'c', 'a')
#define FR_MAXP_TAG FR_BYTES_TO_U32('m', 'a', 'x', 'p')

typedef struct FrFontFile
{
	FrFileReader reader;
	FrFont* pFont;

	uint32_t cmapOffset;
	uint32_t glyfOffset;
	uint32_t headOffset;
	uint32_t hheaOffset;
	uint32_t hmtxOffset;
	uint32_t locaOffset;
	uint32_t maxpOffset;

	uint32_t contourInfoCapacity;

	uint32_t* offsets;

	uint32_t flagsCapacity;
	uint8_t* flags;

	uint16_t currentGlyph;
	bool parent;
	uint16_t currentChild;

	uint16_t advanceWidthCount;
} FrFontFile;

static FrResult frParseTableDirectory(FrFontFile* pFontFile)
{
	if(!pFontFile)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(frSkipBytes(&pFontFile->reader, 4) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	uint16_t numTables;
	if(frReadUint16(&pFontFile->reader, &numTables) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	if(frSkipBytes(&pFontFile->reader, 6) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	for(uint16_t tableIndex = 0; tableIndex < numTables; ++tableIndex)
	{
		uint32_t tag;
		if(frReadUint32(&pFontFile->reader, &tag) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		if(frSkipBytes(&pFontFile->reader, 4) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		uint32_t offset;
		if(frReadUint32(&pFontFile->reader, &offset) != FR_SUCCESS || offset > LONG_MAX)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		switch(tag)
		{
			case FR_CMAP_TAG:
				pFontFile->cmapOffset = offset;
				break;

			case FR_GLYF_TAG:
				pFontFile->glyfOffset = offset;
				break;

			case FR_HEAD_TAG:
				pFontFile->headOffset = offset;
				break;

			case FR_HHEA_TAG:
				pFontFile->hheaOffset = offset;
				break;

			case FR_HMTX_TAG:
				pFontFile->hmtxOffset = offset;
				break;

			case FR_LOCA_TAG:
				pFontFile->locaOffset = offset;
				break;

			case FR_MAXP_TAG:
				pFontFile->maxpOffset = offset;
				break;

			default:
				break;
		}

		if(frSkipBytes(&pFontFile->reader, 4) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
	}

	if(
		pFontFile->cmapOffset == 0 ||
		pFontFile->glyfOffset == 0 ||
		pFontFile->headOffset == 0 ||
		pFontFile->locaOffset == 0 ||
		pFontFile->maxpOffset == 0
	)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	return FR_SUCCESS;
}

static FrResult frParseSimpleGlyph(FrFontFile* pFile, uint16_t contourCount)
{
	const uint16_t id = pFile->parent ? pFile->currentGlyph : pFile->currentChild;

	const int16_t xMin = pFile->pFont->glyphPositions[id].xMin;
	const int16_t yMin = pFile->pFont->glyphPositions[id].yMin;
	const int16_t xMax = pFile->pFont->glyphPositions[id].xMax;
	const int16_t yMax = pFile->pFont->glyphPositions[id].yMax;

	const float widthInv = 1.f / (xMax - xMin);
	const float heightInv = 1.f / (yMax - yMin);

	// Read contour info
	const uint16_t contourInfoOffset = pFile->pFont->contourInfoCount;
	for(uint16_t i = 0; i < contourCount; ++i)
	{
		uint16_t contourInfo;
		if(frReadUint16(&pFile->reader, &contourInfo) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		pFile->pFont->contourInfos[pFile->pFont->contourInfoCount] = contourInfo;
		++pFile->pFont->contourInfoCount;
	}

	// Skip instructions
	uint16_t instructionLength;
	if(frReadUint16(&pFile->reader, &instructionLength) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	if(frSkipBytes(&pFile->reader, instructionLength) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Read flags
	const uint32_t pointCount = pFile->pFont->contourInfos[pFile->pFont->contourInfoCount - 1] + 1;
	if(pointCount > pFile->flagsCapacity)
	{
		if(frResize(&pFile->flags, pointCount * sizeof(pFile->flags[0])) != FR_SUCCESS)
		{
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
		pFile->flagsCapacity = pointCount;
	}
	for(uint32_t i = 0; i < pointCount; ++i)
	{
		if(frReadUint8(&pFile->reader, &pFile->flags[i]) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		if(FR_BIT(pFile->flags[i], 3))
		{
			uint8_t repeatCount;
			if(frReadUint8(&pFile->reader, &repeatCount) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			if(repeatCount > pFile->pFont->contourInfos[pFile->pFont->contourInfoCount - 1] - i)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			for(uint32_t j = 0; j < repeatCount; ++j)
			{
				pFile->flags[i + 1 + j] = pFile->flags[i];
			}
			i += repeatCount;
		}
	}

	// Read x coordinates
	uint32_t firstPoint = 0;
	uint32_t glyphPointCount = 0;
	uint32_t lastOffset = 0;
	float lastX = -xMin * widthInv;
	for(uint16_t contourIndex = 0; contourIndex < contourCount; ++contourIndex)
	{
		const uint32_t lastPoint = pFile->pFont->contourInfos[contourInfoOffset + contourIndex];

		for(uint32_t i = firstPoint; i <= lastPoint; ++i)
		{
			float x = lastX;

			if(FR_BIT(pFile->flags[i], 1))
			{
				uint8_t coordinate;
				if(frReadUint8(&pFile->reader, &coordinate) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				const int16_t sign = FR_BIT(pFile->flags[i], 4) * 2 - 1;
				const int16_t newX16 = sign * coordinate;
				x += newX16 * widthInv;
			}
			else if(!FR_BIT(pFile->flags[i], 4))
			{
				int16_t delta;
				if(frReadInt16(&pFile->reader, &delta) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				x += delta * widthInv;
			}

			if(i > firstPoint && FR_BIT(pFile->flags[i], 0) == FR_BIT(pFile->flags[i - 1], 0))
			{
				const float inBetween = (x + lastX) / 2;
				pFile->pFont->points[pFile->pFont->pointCount + glyphPointCount].x = inBetween;
				++glyphPointCount;
			}

			pFile->pFont->points[pFile->pFont->pointCount + glyphPointCount].x = x;
			++glyphPointCount;

			lastX = x;
		}

		if(FR_BIT(pFile->flags[lastPoint], 0) == FR_BIT(pFile->flags[firstPoint], 0))
		{
			pFile->pFont->points[pFile->pFont->pointCount + glyphPointCount].x = (pFile->pFont->points[pFile->pFont->pointCount + lastOffset].x + lastX) / 2;
			++glyphPointCount;
		}

		firstPoint = lastPoint + 1;
		lastOffset = glyphPointCount;
	}

	// Read y coordinates
	firstPoint = 0;
	glyphPointCount = 0;
	lastOffset = 0;
	float lastY = -yMin * heightInv;
	uint16_t contourPointCount = 0;
	for(uint16_t contourIndex = 0; contourIndex < contourCount; ++contourIndex)
	{
		const uint32_t lastPoint = pFile->pFont->contourInfos[contourInfoOffset + contourIndex];

		for(uint32_t i = firstPoint; i <= lastPoint; ++i)
		{
			float y = lastY;

			if(FR_BIT(pFile->flags[i], 2))
			{
				uint8_t coordinate;
				if(frReadUint8(&pFile->reader, &coordinate) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				const int16_t sign = FR_BIT(pFile->flags[i], 5) * 2 - 1;
				const int16_t newY16 = sign * coordinate;
				y += newY16 * heightInv;
			}
			else if(!FR_BIT(pFile->flags[i], 5))
			{
				int16_t delta;
				if(frReadInt16(&pFile->reader, &delta) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				y += delta * heightInv;
			}

			if(i > firstPoint && FR_BIT(pFile->flags[i], 0) == FR_BIT(pFile->flags[i - 1], 0))
			{
				const float inBetween = (y + lastY) / 2;
				pFile->pFont->points[pFile->pFont->pointCount + glyphPointCount].y = inBetween;
				++glyphPointCount;
				++contourPointCount;
			}

			pFile->pFont->points[pFile->pFont->pointCount + glyphPointCount].y = y;
			++glyphPointCount;
			++contourPointCount;

			lastY = y;
		}

		if(FR_BIT(pFile->flags[lastPoint], 0) == FR_BIT(pFile->flags[firstPoint], 0))
		{
			pFile->pFont->points[pFile->pFont->pointCount + glyphPointCount].y = (pFile->pFont->points[pFile->pFont->pointCount + lastOffset].y + lastY) / 2;
			++glyphPointCount;
			++contourPointCount;
		}

		firstPoint = lastPoint + 1;
		lastOffset = glyphPointCount;

		pFile->pFont->contourInfos[contourInfoOffset + contourIndex] = contourPointCount;
		contourPointCount = 0;
	}

	pFile->pFont->pointCount += glyphPointCount;

	return FR_SUCCESS;
}

static FrResult frParseGlyph(FrFontFile* pFile);

static FrResult frParseCompositeGlyph(FrFontFile* pFile)
{
	uint16_t flags;
	do
	{
		if(frReadUint16(&pFile->reader, &flags) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		uint16_t glyphIndex;
		if(frReadUint16(&pFile->reader, &glyphIndex) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		float e, f;
		if(FR_BIT(flags, 1))
		{
			if(FR_BIT(flags, 0))
			{
				int16_t arg1, arg2;
				if(frReadInt16(&pFile->reader, &arg1) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				if(frReadInt16(&pFile->reader, &arg2) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				e = arg1;
				f = arg2;
			}
			else
			{
				int8_t arg1, arg2;
				if(frReadInt8(&pFile->reader, &arg1) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				if(frReadInt8(&pFile->reader, &arg2) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				e = arg1;
				f = arg2;
			}
		}
		else
		{
			// TODO
			printf("TODO\n");
			return FR_ERROR_CORRUPTED_FILE;
		}

		float a, b, c, d;
		if(FR_BIT(flags, 3))
		{
			if(frReadF2d14(&pFile->reader, &a) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			b = 0;
			c = 0;
			d = a;
		}
		else if(FR_BIT(flags, 6))
		{
			if(frReadF2d14(&pFile->reader, &a) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			if(frReadF2d14(&pFile->reader, &d) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			b = 0;
			c = 0;
		}
		else if(FR_BIT(flags, 7))
		{
			if(frReadF2d14(&pFile->reader, &a) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			if(frReadF2d14(&pFile->reader, &b) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			if(frReadF2d14(&pFile->reader, &c) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			if(frReadF2d14(&pFile->reader, &d) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
		}
		else
		{
			a = 1;
			b = 0;
			c = 0;
			d = 1;
		}
		const float absA = fabsf(a);
		const float absB = fabsf(b);
		const float absC = fabsf(c);
		const float absD = fabsf(d);

		const float m0 = absA < absB ? absB : absA;
		const float n0 = absC < absD ? absD : absC;

		const float m = (1 + (fabsf(absA - absC) <= 33 / 65536.f)) * m0;
		const float n = (1 + (fabsf(absB - absD) <= 33 / 65536.f)) * n0;

		const long currentOffset = ftell(pFile->reader.file);
		const uint32_t startPointCount = pFile->pFont->pointCount;
		pFile->currentChild = glyphIndex;
		if(frMoveTo(&pFile->reader, pFile->glyfOffset + pFile->offsets[glyphIndex]) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		if(frParseGlyph(pFile) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		if(frMoveTo(&pFile->reader, currentOffset) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		const uint32_t endPointCount = pFile->pFont->pointCount;

		for(uint32_t i = startPointCount; i < endPointCount; ++i)
		{
			const float x = pFile->pFont->points[i].x * (pFile->pFont->glyphPositions[glyphIndex].xMax - pFile->pFont->glyphPositions[glyphIndex].xMin) + pFile->pFont->glyphPositions[glyphIndex].xMin;
			const float y = pFile->pFont->points[i].y * (pFile->pFont->glyphPositions[glyphIndex].yMax - pFile->pFont->glyphPositions[glyphIndex].yMin) + pFile->pFont->glyphPositions[glyphIndex].yMin;

			pFile->pFont->points[i].x = (m * (a / m * x + c / m * y + e) - pFile->pFont->glyphPositions[pFile->currentGlyph].xMin) / (pFile->pFont->glyphPositions[pFile->currentGlyph].xMax - pFile->pFont->glyphPositions[pFile->currentGlyph].xMin);
			pFile->pFont->points[i].y = (n * (b / n * x + d / n * y + f) - pFile->pFont->glyphPositions[pFile->currentGlyph].yMin) / (pFile->pFont->glyphPositions[pFile->currentGlyph].yMax - pFile->pFont->glyphPositions[pFile->currentGlyph].yMin);
		}
	} while(FR_BIT(flags, 5));

	return FR_SUCCESS;
}

static FrResult frParseGlyph(FrFontFile* pFile)
{
	int16_t contourCount;
	if(frReadInt16(&pFile->reader, &contourCount) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	const uint16_t id = pFile->parent ? pFile->currentGlyph : pFile->currentChild;
	if(frReadInt16(&pFile->reader, &pFile->pFont->glyphPositions[id].xMin) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	if(frReadInt16(&pFile->reader, &pFile->pFont->glyphPositions[id].yMin) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	if(frReadInt16(&pFile->reader, &pFile->pFont->glyphPositions[id].xMax) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	if(frReadInt16(&pFile->reader, &pFile->pFont->glyphPositions[id].yMax) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Composite glyph
	if(contourCount < 0)
	{
		if((uint32_t)pFile->pFont->contourInfoCount + 1 >= pFile->contourInfoCapacity)
		{
			pFile->contourInfoCapacity *= 2;
			if(frResize(&pFile->pFont->contourInfos, pFile->contourInfoCapacity * sizeof(pFile->pFont->contourInfos[0])) != FR_SUCCESS)
			{
				return FR_ERROR_OUT_OF_HOST_MEMORY;
			}
		}

		const uint16_t contourCountIndex = pFile->pFont->contourInfoCount;
		bool parent = pFile->parent;
		pFile->parent = false;

		if(parent)
		{
			++pFile->pFont->contourInfoCount;
		}

		if(frParseCompositeGlyph(pFile) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		if(parent)
		{
			pFile->pFont->contourInfos[contourCountIndex] = pFile->pFont->contourInfoCount - contourCountIndex - 1;
		}

		return FR_SUCCESS;
	}

	// Simple glyph
	if((uint32_t)pFile->pFont->contourInfoCount + contourCount >= pFile->contourInfoCapacity)
	{
		pFile->contourInfoCapacity *= 2;
		if(frResize(&pFile->pFont->contourInfos, pFile->contourInfoCapacity * sizeof(pFile->pFont->contourInfos[0])) != FR_SUCCESS)
		{
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
	}
	if(pFile->parent)
	{
		pFile->pFont->contourInfos[pFile->pFont->contourInfoCount] = contourCount;
		++pFile->pFont->contourInfoCount;
	}

	if(frParseSimpleGlyph(pFile, contourCount) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	return FR_SUCCESS;
}

static FrResult frParseGlyphs(FrFontFile* pFile)
{
	pFile->flags = NULL;
	pFile->flagsCapacity = 0;

	pFile->pFont->glyphPositions = malloc(pFile->pFont->glyphCount * sizeof(pFile->pFont->glyphPositions[0]));
	if(!pFile->pFont->glyphPositions)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	pFile->pFont->glyphOffsets = malloc(pFile->pFont->glyphCount * 2 * sizeof(pFile->pFont->glyphOffsets[0]));
	if(!pFile->pFont->glyphOffsets)
	{
		free(pFile->pFont->glyphPositions);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	// hhea
	if(frMoveTo(&pFile->reader, pFile->hheaOffset + 34) != FR_SUCCESS)
	{
		free(pFile->pFont->glyphPositions);
		free(pFile->pFont->glyphOffsets);
		return FR_ERROR_CORRUPTED_FILE;
	}
	if(frReadUint16(&pFile->reader, &pFile->advanceWidthCount) != FR_SUCCESS)
	{
		free(pFile->pFont->glyphPositions);
		free(pFile->pFont->glyphOffsets);
		return FR_ERROR_CORRUPTED_FILE;
	}

	// hmtx
	if(frMoveTo(&pFile->reader, pFile->hmtxOffset) != FR_SUCCESS)
	{
		free(pFile->pFont->glyphPositions);
		free(pFile->pFont->glyphOffsets);
		return FR_ERROR_CORRUPTED_FILE;
	}
	for(uint32_t i = 0; i < pFile->pFont->glyphCount; ++i)
	{
		if(i < pFile->advanceWidthCount)
		{
			if(frReadUint16(&pFile->reader, &pFile->pFont->glyphPositions[i].advanceWidth) != FR_SUCCESS)
			{
				free(pFile->pFont->glyphPositions);
				free(pFile->pFont->glyphOffsets);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}
		else
		{
			pFile->pFont->glyphPositions[i].advanceWidth = pFile->pFont->glyphPositions[pFile->advanceWidthCount - 1].advanceWidth;
		}
		if(frReadInt16(&pFile->reader, &pFile->pFont->glyphPositions[i].leftSideBearing) != FR_SUCCESS)
		{
			free(pFile->pFont->glyphPositions);
			free(pFile->pFont->glyphOffsets);
			return FR_ERROR_CORRUPTED_FILE;
		}
	}

	for(uint16_t i = 0; i < pFile->pFont->glyphCount; ++i)
	{
		if(pFile->offsets[i] == pFile->offsets[(uint32_t)i + 1])
		{
			pFile->pFont->glyphPositions[i].xMin = 0;
			pFile->pFont->glyphPositions[i].yMin = 0;
			pFile->pFont->glyphPositions[i].xMax = 0;
			pFile->pFont->glyphPositions[i].yMax = 0;

			continue;
		}

		pFile->currentGlyph = i;
		pFile->parent = true;

		pFile->pFont->glyphOffsets[2 * (uint32_t)i] = pFile->pFont->contourInfoCount;
		pFile->pFont->glyphOffsets[2 * (uint32_t)i + 1] = pFile->pFont->pointCount;

		if(frMoveTo(&pFile->reader, pFile->glyfOffset + pFile->offsets[i]) != FR_SUCCESS)
		{
			free(pFile->pFont->glyphPositions);
			free(pFile->flags);
			return FR_ERROR_CORRUPTED_FILE;
		}

		if(frParseGlyph(pFile) != FR_SUCCESS)
		{
			free(pFile->pFont->glyphPositions);
			free(pFile->flags);
			return FR_ERROR_CORRUPTED_FILE;
		}
	}

	free(pFile->flags);

	return FR_SUCCESS;
}

static FrResult frParseMappings(FrFontFile* pFile)
{
	if(frMoveTo(&pFile->reader, pFile->cmapOffset + 2) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	uint16_t numSubtables;
	if(frReadUint16(&pFile->reader, &numSubtables) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	uint16_t platformId = UINT16_MAX, encodingId = 0;
	uint32_t offset = 0;

	for(uint16_t i = 0; i < numSubtables; ++i)
	{
		uint16_t currentPlatformId, currentEncodingId;
		if(frReadUint16(&pFile->reader, &currentPlatformId) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		if(frReadUint16(&pFile->reader, &currentEncodingId) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		if(currentPlatformId < platformId)
		{
			platformId = currentPlatformId;
			encodingId = currentEncodingId;
			if(frReadUint32(&pFile->reader, &offset) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}

			if(currentPlatformId > 0)
			{
				break;
			}

			continue;
		}

		if(currentPlatformId != platformId)
		{
			break;
		}

		if(platformId == 0 && currentEncodingId > encodingId && currentEncodingId <= 4)
		{
			encodingId = currentEncodingId;
			if(frReadUint32(&pFile->reader, &offset) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
		}
	}
	if(frMoveTo(&pFile->reader, pFile->cmapOffset + offset) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	uint16_t format;
	if(frReadUint16(&pFile->reader, &format) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	if(format == FR_CMAP_FORMAT_4)
	{
		uint16_t length;
		if(frReadUint16(&pFile->reader, &length) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		pFile->pFont->cmapFormat = FR_CMAP_FORMAT_4;
		if(length % sizeof(length) != 0 || length <= 14)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		const uint16_t cmapDataSize = length - 14 + 2;
		uint16_t* const cmapData = malloc(cmapDataSize);
		if(!cmapData)
		{
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}

		if(frSkipBytes(&pFile->reader, 2) != FR_SUCCESS)
		{
			free(cmapData);
			return FR_ERROR_CORRUPTED_FILE;
		}

		if(frReadUint16(&pFile->reader, &cmapData[0]) != FR_SUCCESS)
		{
			free(cmapData);
			return FR_ERROR_CORRUPTED_FILE;
		}
		cmapData[0] /= 2;

		if(frSkipBytes(&pFile->reader, 6) != FR_SUCCESS)
		{
			free(cmapData);
			return FR_ERROR_CORRUPTED_FILE;
		}

		for(uint16_t j = 0; j < cmapData[0]; ++j)
		{
			if(frReadUint16(&pFile->reader, &cmapData[j + 1]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}

		if(frSkipBytes(&pFile->reader, 2) != FR_SUCCESS)
		{
			free(cmapData);
			return FR_ERROR_CORRUPTED_FILE;
		}

		for(uint16_t j = 0; j < cmapData[0]; ++j)
		{
			if(frReadUint16(&pFile->reader, &cmapData[j + cmapData[0] + 1]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}

		for(uint16_t j = 0; j < cmapData[0]; ++j)
		{
			if(frReadUint16(&pFile->reader, &cmapData[j + 2 * cmapData[0] + 1]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}

		for(uint16_t j = 0; j < cmapData[0]; ++j)
		{
			if(frReadUint16(&pFile->reader, &cmapData[j + 3 * cmapData[0] + 1]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}

		const uint16_t glyphIndexCount = cmapDataSize / sizeof(cmapData[0]) - 2 - 4 * cmapData[0];
		cmapData[4 * cmapData[0] + 1] = glyphIndexCount;
		for(uint16_t j = 0; j < glyphIndexCount; ++j)
		{
			if(frReadUint16(&pFile->reader, &cmapData[j + 4 * cmapData[0] + 2]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}

		pFile->pFont->cmapData = cmapData;

		return FR_SUCCESS;
	}

	if(format == FR_CMAP_FORMAT_12)
	{
		pFile->pFont->cmapFormat = FR_CMAP_FORMAT_12;

		if(frSkipBytes(&pFile->reader, 10) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		uint32_t numGroups;
		if(frReadUint32(&pFile->reader, &numGroups) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		uint32_t* const cmapData = malloc((numGroups * 3 + 1) * sizeof(numGroups));
		if(!cmapData)
		{
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
		cmapData[0] = numGroups;

		for(uint32_t i = 0; i < numGroups; ++i)
		{
			if(frReadUint32(&pFile->reader, &cmapData[1 + i]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}

			if(frReadUint32(&pFile->reader, &cmapData[1 + numGroups + i]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}

			if(frReadUint32(&pFile->reader, &cmapData[1 + 2 * numGroups + i]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}

		pFile->pFont->cmapData = cmapData;

		return FR_SUCCESS;
	}

	return FR_ERROR_CORRUPTED_FILE;
}

FrResult frLoadFont(const char* path, FrFont* pFont)
{
	if(!path || !pFont)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	*pFont = (FrFont){0};
	FrFontFile fontFile = {0};
	fontFile.reader.file = fopen(path, "rb");
	if(!fontFile.reader.file)
	{
		return FR_ERROR_FILE_NOT_FOUND;
	}
	fontFile.pFont = pFont;

	if(frParseTableDirectory(&fontFile) != FR_SUCCESS)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	// head
	if(frMoveTo(&fontFile.reader, fontFile.headOffset + 18) != FR_SUCCESS)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}
	uint16_t unitsPerEm;
	if(frReadUint16(&fontFile.reader, &unitsPerEm) != FR_SUCCESS)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	if(frMoveTo(&fontFile.reader, fontFile.headOffset + 50) != FR_SUCCESS)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	uint16_t indexToLocFormat;
	if(frReadUint16(&fontFile.reader, &indexToLocFormat) != FR_SUCCESS)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	// maxp
	if(frMoveTo(&fontFile.reader, fontFile.maxpOffset + 4) != FR_SUCCESS)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	if(frReadUint16(&fontFile.reader, &fontFile.pFont->glyphCount) != FR_SUCCESS)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	fontFile.contourInfoCapacity = fontFile.pFont->glyphCount * UINT32_C(4);
	fontFile.pFont->contourInfos = malloc(fontFile.contourInfoCapacity * sizeof(fontFile.pFont->contourInfos[0]));
	if(!fontFile.pFont->contourInfos)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	fontFile.pFont->points = malloc(fontFile.pFont->glyphCount * 100 * sizeof(fontFile.pFont->points[0]));
	if(!fontFile.pFont->points)
	{
		free(fontFile.pFont->contourInfos);
		fclose(fontFile.reader.file);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	// loca
	if(frMoveTo(&fontFile.reader, fontFile.locaOffset) != FR_SUCCESS)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	fontFile.offsets = malloc((fontFile.pFont->glyphCount + 1) * sizeof(fontFile.offsets[0]));
	if(!fontFile.offsets)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	for(uint16_t i = 0; i <= fontFile.pFont->glyphCount; ++i)
	{
		if(indexToLocFormat == 0)
		{
			uint16_t offset;
			if(frReadUint16(&fontFile.reader, &offset) != FR_SUCCESS)
			{
				free(fontFile.offsets);
				fclose(fontFile.reader.file);
				return FR_ERROR_CORRUPTED_FILE;
			}
			fontFile.offsets[i] = offset * UINT32_C(2);
		}
		else
		{
			if(frReadUint32(&fontFile.reader, &fontFile.offsets[i]) != FR_SUCCESS)
			{
				free(fontFile.offsets);
				fclose(fontFile.reader.file);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}
	}

	// glyf
	if(frParseGlyphs(&fontFile) != FR_SUCCESS)
	{
		free(fontFile.pFont->points);
		free(fontFile.pFont->contourInfos);
		free(fontFile.offsets);
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	// cmap
	if(frParseMappings(&fontFile) != FR_SUCCESS)
	{
		free(fontFile.pFont->points);
		free(fontFile.pFont->contourInfos);

		free(fontFile.pFont->glyphPositions);
		free(fontFile.pFont->glyphOffsets);

		free(fontFile.offsets);
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	free(fontFile.offsets);

	fclose(fontFile.reader.file);

	return FR_SUCCESS;
}

FrResult frGetGlyphId(const FrFont* pFont, uint32_t characterCode, uint32_t* pGlyphId)
{
	if(!pFont || !pGlyphId)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(pFont->cmapFormat == FR_CMAP_FORMAT_4)
	{
		if(characterCode > UINT16_MAX)
		{
			*pGlyphId = 0;
			return EXIT_SUCCESS;
		}

		const uint16_t segCount = *(uint16_t*)pFont->cmapData;
		const uint16_t* const endCodes = (uint16_t*)pFont->cmapData + 1;
		const uint16_t* const startCodes = endCodes + segCount;
		const uint16_t* const idDeltas = startCodes + segCount;
		const uint16_t* const idRangeOffsets = idDeltas + segCount;
		const uint16_t glyphIndexCount = *(uint16_t*)(idRangeOffsets + segCount);

		const uint16_t character = (uint16_t)characterCode;

		uint16_t i;
		for(i = 0; i < segCount && character > endCodes[i]; ++i);
		if(i == segCount)
		{
			*pGlyphId = 0;
			return FR_SUCCESS;
		}

		if(character < startCodes[i])
		{
			*pGlyphId = 0;
			return FR_SUCCESS;
		}

		if(idRangeOffsets[i] == 0)
		{
			*pGlyphId = (character + idDeltas[i]) % (UINT16_MAX + UINT32_C(1));
			return FR_SUCCESS;
		}

		const uint16_t offset = idRangeOffsets[i] / 2 + (character - startCodes[i]);
		if(offset >= segCount + glyphIndexCount)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		*pGlyphId = *(idRangeOffsets + i + offset + 1);
		if(*pGlyphId == 0)
		{
			return FR_SUCCESS;
		}
		*pGlyphId = (*pGlyphId + idDeltas[i]) % (UINT16_MAX + UINT32_C(1));

		return FR_SUCCESS;
	}

	if(pFont->cmapFormat == FR_CMAP_FORMAT_12)
	{
		const uint32_t numGroups = *(uint32_t*)pFont->cmapData;
		const uint32_t* const startCharCodes = (uint32_t*)pFont->cmapData + 1;
		const uint32_t* const endCharCodes = startCharCodes + numGroups;
		const uint32_t* const startGlyphCodes = endCharCodes + numGroups;

		for(uint32_t i = 0; i < numGroups; ++i)
		{
			if(characterCode >= startCharCodes[i] && characterCode <= endCharCodes[i])
			{
				*pGlyphId = startGlyphCodes[i] + characterCode - startCharCodes[i];
				return FR_SUCCESS;
			}
		}

		return FR_ERROR_CORRUPTED_FILE;
	}

	return FR_ERROR_CORRUPTED_FILE;
}

FrResult frFreeFont(FrFont* pFont)
{
	if(!pFont)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	free(pFont->points);
	free(pFont->contourInfos);

	free(pFont->glyphPositions);
	free(pFont->glyphOffsets);

	free(pFont->cmapData);

	return FR_SUCCESS;
}

#define FR_1BYTE  0x80
#define FR_2BYTES 0xE0
#define FR_3BYTES 0xF0

#define FR_3BITS 0x07
#define FR_4BITS 0x0F
#define FR_5BITS 0x1F
#define FR_6BITS 0x3F
#define FR_7BITS 0x7F

FrResult frNextCharacterCode(FrStringReader* pStringReader, uint32_t* pCharacterCode)
{
	static_assert(CHAR_BIT == 8, "Only 8 bits chars are supported for now");

	if(!pStringReader || !pStringReader->string || !pCharacterCode)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(pStringReader->string >= pStringReader->end)
	{
		*pCharacterCode = 0;
		return FR_SUCCESS;
	}

	const uint8_t first = *pStringReader->string;
	if(first < FR_1BYTE)
	{
		*pCharacterCode = first & FR_7BITS;
		++pStringReader->string;
		return FR_SUCCESS;
	}

	if(pStringReader->string + 1 >= pStringReader->end)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(first < FR_2BYTES)
	{
		*pCharacterCode = first & FR_5BITS;
		*pCharacterCode <<= 6;
		++pStringReader->string;

		*pCharacterCode |= *pStringReader->string & FR_6BITS;
		++pStringReader->string;

		return FR_SUCCESS;
	}

	if(pStringReader->string + 2 >= pStringReader->end)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(first < FR_3BYTES)
	{
		*pCharacterCode = first & FR_4BITS;
		*pCharacterCode <<= 6;
		++pStringReader->string;

		*pCharacterCode |= *pStringReader->string & FR_6BITS;
		*pCharacterCode <<= 6;
		++pStringReader->string;

		*pCharacterCode |= *pStringReader->string & FR_6BITS;
		*pCharacterCode <<= 6;
		++pStringReader->string;

		return FR_SUCCESS;
	}

	if(pStringReader->string + 3 >= pStringReader->end)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	*pCharacterCode = first & FR_3BITS;
	*pCharacterCode <<= 6;
	++pStringReader->string;

	*pCharacterCode |= *pStringReader->string & FR_6BITS;
	*pCharacterCode <<= 6;
	++pStringReader->string;

	*pCharacterCode |= *pStringReader->string & FR_6BITS;
	*pCharacterCode <<= 6;
	++pStringReader->string;

	*pCharacterCode |= *pStringReader->string & FR_6BITS;
	*pCharacterCode <<= 6;
	++pStringReader->string;

	return FR_SUCCESS;
}
