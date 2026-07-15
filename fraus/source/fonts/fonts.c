#include "fraus/fonts/fonts.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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
	FrFont* font;

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

static FrResult frParseTableDirectory(FrFontFile* const fontFile)
{
	if(!fontFile)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(frSkipBytes(&fontFile->reader, 4) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	uint16_t numTables;
	if(frReadUint16(&fontFile->reader, &numTables) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	if(frSkipBytes(&fontFile->reader, 6) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	for(uint16_t tableIndex = 0; tableIndex < numTables; ++tableIndex)
	{
		uint32_t tag;
		if(frReadUint32(&fontFile->reader, &tag) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		if(frSkipBytes(&fontFile->reader, 4) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		uint32_t offset;
		if(frReadUint32(&fontFile->reader, &offset) != FR_SUCCESS || offset > LONG_MAX)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		switch(tag)
		{
			case FR_CMAP_TAG:
				fontFile->cmapOffset = offset;
				break;

			case FR_GLYF_TAG:
				fontFile->glyfOffset = offset;
				break;

			case FR_HEAD_TAG:
				fontFile->headOffset = offset;
				break;

			case FR_HHEA_TAG:
				fontFile->hheaOffset = offset;
				break;

			case FR_HMTX_TAG:
				fontFile->hmtxOffset = offset;
				break;

			case FR_LOCA_TAG:
				fontFile->locaOffset = offset;
				break;

			case FR_MAXP_TAG:
				fontFile->maxpOffset = offset;
				break;

			default:
				break;
		}

		if(frSkipBytes(&fontFile->reader, 4) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
	}

	if(
		fontFile->cmapOffset == 0 ||
		fontFile->glyfOffset == 0 ||
		fontFile->headOffset == 0 ||
		fontFile->locaOffset == 0 ||
		fontFile->maxpOffset == 0
	)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	return FR_SUCCESS;
}

static FrResult frParseSimpleGlyph(FrFontFile* const file, const uint16_t contourCount)
{
	const uint16_t id = file->parent ? file->currentGlyph : file->currentChild;

	const int16_t xMin = file->font->glyphPositions[id].xMin;
	const int16_t yMin = file->font->glyphPositions[id].yMin;
	const int16_t xMax = file->font->glyphPositions[id].xMax;
	const int16_t yMax = file->font->glyphPositions[id].yMax;

	const float widthInv = 1.f / (xMax - xMin);
	const float heightInv = 1.f / (yMax - yMin);

	// Read contour info
	const uint16_t contourInfoOffset = file->font->contourInfoCount;
	for(uint16_t i = 0; i < contourCount; ++i)
	{
		uint16_t contourInfo;
		if(frReadUint16(&file->reader, &contourInfo) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		file->font->contourInfos[file->font->contourInfoCount] = contourInfo;
		++file->font->contourInfoCount;
	}

	// Skip instructions
	uint16_t instructionLength;
	if(frReadUint16(&file->reader, &instructionLength) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	if(frSkipBytes(&file->reader, instructionLength) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Read flags
	const uint32_t pointCount = file->font->contourInfos[file->font->contourInfoCount - 1] + 1;
	if(pointCount > file->flagsCapacity)
	{
		uint8_t* const newFlags = realloc(file->flags, pointCount * sizeof(file->flags[0]));
		if(!newFlags)
		{
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
		file->flags = newFlags;
		file->flagsCapacity = pointCount;
	}
	for(uint32_t i = 0; i < pointCount; ++i)
	{
		if(frReadUint8(&file->reader, &file->flags[i]) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		if(FR_BIT(file->flags[i], 3))
		{
			uint8_t repeatCount;
			if(frReadUint8(&file->reader, &repeatCount) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			if(repeatCount > file->font->contourInfos[file->font->contourInfoCount - 1] - i)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			for(uint32_t j = 0; j < repeatCount; ++j)
			{
				file->flags[i + 1 + j] = file->flags[i];
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
		const uint32_t lastPoint = file->font->contourInfos[contourInfoOffset + contourIndex];

		for(uint32_t i = firstPoint; i <= lastPoint; ++i)
		{
			float x = lastX;

			if(FR_BIT(file->flags[i], 1))
			{
				uint8_t coordinate;
				if(frReadUint8(&file->reader, &coordinate) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				const int16_t sign = FR_BIT(file->flags[i], 4) * 2 - 1;
				const int16_t newX16 = sign * coordinate;
				x += newX16 * widthInv;
			}
			else if(!FR_BIT(file->flags[i], 4))
			{
				int16_t delta;
				if(frReadInt16(&file->reader, &delta) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				x += delta * widthInv;
			}

			if(i > firstPoint && FR_BIT(file->flags[i], 0) == FR_BIT(file->flags[i - 1], 0))
			{
				const float inBetween = (x + lastX) / 2;
				file->font->points[file->font->pointCount + glyphPointCount].x = inBetween;
				++glyphPointCount;
			}

			file->font->points[file->font->pointCount + glyphPointCount].x = x;
			++glyphPointCount;

			lastX = x;
		}

		if(FR_BIT(file->flags[lastPoint], 0) == FR_BIT(file->flags[firstPoint], 0))
		{
			file->font->points[file->font->pointCount + glyphPointCount].x = (file->font->points[file->font->pointCount + lastOffset].x + lastX) / 2;
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
		const uint32_t lastPoint = file->font->contourInfos[contourInfoOffset + contourIndex];

		for(uint32_t i = firstPoint; i <= lastPoint; ++i)
		{
			float y = lastY;

			if(FR_BIT(file->flags[i], 2))
			{
				uint8_t coordinate;
				if(frReadUint8(&file->reader, &coordinate) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				const int16_t sign = FR_BIT(file->flags[i], 5) * 2 - 1;
				const int16_t newY16 = sign * coordinate;
				y += newY16 * heightInv;
			}
			else if(!FR_BIT(file->flags[i], 5))
			{
				int16_t delta;
				if(frReadInt16(&file->reader, &delta) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				y += delta * heightInv;
			}

			if(i > firstPoint && FR_BIT(file->flags[i], 0) == FR_BIT(file->flags[i - 1], 0))
			{
				const float inBetween = (y + lastY) / 2;
				file->font->points[file->font->pointCount + glyphPointCount].y = inBetween;
				++glyphPointCount;
				++contourPointCount;
			}

			file->font->points[file->font->pointCount + glyphPointCount].y = y;
			++glyphPointCount;
			++contourPointCount;

			lastY = y;
		}

		if(FR_BIT(file->flags[lastPoint], 0) == FR_BIT(file->flags[firstPoint], 0))
		{
			file->font->points[file->font->pointCount + glyphPointCount].y = (file->font->points[file->font->pointCount + lastOffset].y + lastY) / 2;
			++glyphPointCount;
			++contourPointCount;
		}

		firstPoint = lastPoint + 1;
		lastOffset = glyphPointCount;

		file->font->contourInfos[contourInfoOffset + contourIndex] = contourPointCount;
		contourPointCount = 0;
	}

	file->font->pointCount += glyphPointCount;

	return FR_SUCCESS;
}

static FrResult frParseGlyph(FrFontFile* file);

static FrResult frParseCompositeGlyph(FrFontFile* const file)
{
	uint16_t flags;
	do
	{
		if(frReadUint16(&file->reader, &flags) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		uint16_t glyphIndex;
		if(frReadUint16(&file->reader, &glyphIndex) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		float e, f;
		if(FR_BIT(flags, 1))
		{
			if(FR_BIT(flags, 0))
			{
				int16_t arg1, arg2;
				if(frReadInt16(&file->reader, &arg1) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				if(frReadInt16(&file->reader, &arg2) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				e = arg1;
				f = arg2;
			}
			else
			{
				int8_t arg1, arg2;
				if(frReadInt8(&file->reader, &arg1) != FR_SUCCESS)
				{
					return FR_ERROR_CORRUPTED_FILE;
				}
				if(frReadInt8(&file->reader, &arg2) != FR_SUCCESS)
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
			if(frReadF2d14(&file->reader, &a) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			b = 0;
			c = 0;
			d = a;
		}
		else if(FR_BIT(flags, 6))
		{
			if(frReadF2d14(&file->reader, &a) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			if(frReadF2d14(&file->reader, &d) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			b = 0;
			c = 0;
		}
		else if(FR_BIT(flags, 7))
		{
			if(frReadF2d14(&file->reader, &a) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			if(frReadF2d14(&file->reader, &b) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			if(frReadF2d14(&file->reader, &c) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
			if(frReadF2d14(&file->reader, &d) != FR_SUCCESS)
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

		const long currentOffset = ftell(file->reader.file);
		const uint32_t startPointCount = file->font->pointCount;
		file->currentChild = glyphIndex;
		if(frMoveTo(&file->reader, file->glyfOffset + file->offsets[glyphIndex]) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		if(frParseGlyph(file) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		if(frMoveTo(&file->reader, currentOffset) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		const uint32_t endPointCount = file->font->pointCount;

		for(uint32_t i = startPointCount; i < endPointCount; ++i)
		{
			const float x = file->font->points[i].x * (file->font->glyphPositions[glyphIndex].xMax - file->font->glyphPositions[glyphIndex].xMin) + file->font->glyphPositions[glyphIndex].xMin;
			const float y = file->font->points[i].y * (file->font->glyphPositions[glyphIndex].yMax - file->font->glyphPositions[glyphIndex].yMin) + file->font->glyphPositions[glyphIndex].yMin;

			file->font->points[i].x = (m * (a / m * x + c / m * y + e) - file->font->glyphPositions[file->currentGlyph].xMin) / (file->font->glyphPositions[file->currentGlyph].xMax - file->font->glyphPositions[file->currentGlyph].xMin);
			file->font->points[i].y = (n * (b / n * x + d / n * y + f) - file->font->glyphPositions[file->currentGlyph].yMin) / (file->font->glyphPositions[file->currentGlyph].yMax - file->font->glyphPositions[file->currentGlyph].yMin);
		}
	} while(FR_BIT(flags, 5));

	return FR_SUCCESS;
}

static FrResult frParseGlyph(FrFontFile* const file)
{
	int16_t contourCount;
	if(frReadInt16(&file->reader, &contourCount) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	const uint16_t id = file->parent ? file->currentGlyph : file->currentChild;
	if(frReadInt16(&file->reader, &file->font->glyphPositions[id].xMin) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	if(frReadInt16(&file->reader, &file->font->glyphPositions[id].yMin) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	if(frReadInt16(&file->reader, &file->font->glyphPositions[id].xMax) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}
	if(frReadInt16(&file->reader, &file->font->glyphPositions[id].yMax) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Composite glyph
	if(contourCount < 0)
	{
		if((uint32_t)file->font->contourInfoCount + 1 >= file->contourInfoCapacity)
		{
			file->contourInfoCapacity *= 2;
			uint32_t* const newContourInfos = realloc(file->font->contourInfos, file->contourInfoCapacity * sizeof(file->font->contourInfos[0]));
			if(!newContourInfos)
			{
				return FR_ERROR_OUT_OF_HOST_MEMORY;
			}
			file->font->contourInfos = newContourInfos;
		}

		const uint16_t contourCountIndex = file->font->contourInfoCount;
		bool parent = file->parent;
		file->parent = false;

		if(parent)
		{
			++file->font->contourInfoCount;
		}

		if(frParseCompositeGlyph(file) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		if(parent)
		{
			file->font->contourInfos[contourCountIndex] = file->font->contourInfoCount - contourCountIndex - 1;
		}

		return FR_SUCCESS;
	}

	// Simple glyph
	if((uint32_t)file->font->contourInfoCount + contourCount >= file->contourInfoCapacity)
	{
		file->contourInfoCapacity *= 2;
		uint32_t* const newContourInfos = realloc(file->font->contourInfos, file->contourInfoCapacity * sizeof(file->font->contourInfos[0]));
		if(!newContourInfos)
		{
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
		file->font->contourInfos = newContourInfos;
	}
	if(file->parent)
	{
		file->font->contourInfos[file->font->contourInfoCount] = contourCount;
		++file->font->contourInfoCount;
	}

	if(frParseSimpleGlyph(file, contourCount) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	return FR_SUCCESS;
}

static FrResult frParseGlyphs(FrFontFile* const file)
{
	file->flags = nullptr;
	file->flagsCapacity = 0;

	file->font->glyphPositions = malloc(file->font->glyphCount * sizeof(file->font->glyphPositions[0]));
	if(!file->font->glyphPositions)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	file->font->glyphOffsets = malloc(file->font->glyphCount * 2 * sizeof(file->font->glyphOffsets[0]));
	if(!file->font->glyphOffsets)
	{
		free(file->font->glyphPositions);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	// hhea
	if(frMoveTo(&file->reader, file->hheaOffset + 34) != FR_SUCCESS)
	{
		free(file->font->glyphPositions);
		free(file->font->glyphOffsets);
		return FR_ERROR_CORRUPTED_FILE;
	}
	if(frReadUint16(&file->reader, &file->advanceWidthCount) != FR_SUCCESS)
	{
		free(file->font->glyphPositions);
		free(file->font->glyphOffsets);
		return FR_ERROR_CORRUPTED_FILE;
	}

	// hmtx
	if(frMoveTo(&file->reader, file->hmtxOffset) != FR_SUCCESS)
	{
		free(file->font->glyphPositions);
		free(file->font->glyphOffsets);
		return FR_ERROR_CORRUPTED_FILE;
	}
	for(uint32_t i = 0; i < file->font->glyphCount; ++i)
	{
		if(i < file->advanceWidthCount)
		{
			if(frReadUint16(&file->reader, &file->font->glyphPositions[i].advanceWidth) != FR_SUCCESS)
			{
				free(file->font->glyphPositions);
				free(file->font->glyphOffsets);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}
		else
		{
			file->font->glyphPositions[i].advanceWidth = file->font->glyphPositions[file->advanceWidthCount - 1].advanceWidth;
		}
		if(frReadInt16(&file->reader, &file->font->glyphPositions[i].leftSideBearing) != FR_SUCCESS)
		{
			free(file->font->glyphPositions);
			free(file->font->glyphOffsets);
			return FR_ERROR_CORRUPTED_FILE;
		}
	}

	for(uint16_t i = 0; i < file->font->glyphCount; ++i)
	{
		if(file->offsets[i] == file->offsets[(uint32_t)i + 1])
		{
			file->font->glyphPositions[i].xMin = 0;
			file->font->glyphPositions[i].yMin = 0;
			file->font->glyphPositions[i].xMax = 0;
			file->font->glyphPositions[i].yMax = 0;

			continue;
		}

		file->currentGlyph = i;
		file->parent = true;

		file->font->glyphOffsets[2 * (uint32_t)i] = file->font->contourInfoCount;
		file->font->glyphOffsets[2 * (uint32_t)i + 1] = file->font->pointCount;

		if(frMoveTo(&file->reader, file->glyfOffset + file->offsets[i]) != FR_SUCCESS)
		{
			free(file->font->glyphPositions);
			free(file->flags);
			return FR_ERROR_CORRUPTED_FILE;
		}

		if(frParseGlyph(file) != FR_SUCCESS)
		{
			free(file->font->glyphPositions);
			free(file->flags);
			return FR_ERROR_CORRUPTED_FILE;
		}
	}

	free(file->flags);

	return FR_SUCCESS;
}

static FrResult frParseMappings(FrFontFile* const file)
{
	if(frMoveTo(&file->reader, file->cmapOffset + 2) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	uint16_t numSubtables;
	if(frReadUint16(&file->reader, &numSubtables) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	uint16_t platformId = UINT16_MAX, encodingId = 0;
	uint32_t offset = 0;

	for(uint16_t i = 0; i < numSubtables; ++i)
	{
		uint16_t currentPlatformId, currentEncodingId;
		if(frReadUint16(&file->reader, &currentPlatformId) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		if(frReadUint16(&file->reader, &currentEncodingId) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		if(currentPlatformId < platformId)
		{
			platformId = currentPlatformId;
			encodingId = currentEncodingId;
			if(frReadUint32(&file->reader, &offset) != FR_SUCCESS)
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
			if(frReadUint32(&file->reader, &offset) != FR_SUCCESS)
			{
				return FR_ERROR_CORRUPTED_FILE;
			}
		}
	}
	if(frMoveTo(&file->reader, file->cmapOffset + offset) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	uint16_t format;
	if(frReadUint16(&file->reader, &format) != FR_SUCCESS)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	if(format == FR_CMAP_FORMAT_4)
	{
		uint16_t length;
		if(frReadUint16(&file->reader, &length) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		file->font->cmapFormat = FR_CMAP_FORMAT_4;
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

		if(frSkipBytes(&file->reader, 2) != FR_SUCCESS)
		{
			free(cmapData);
			return FR_ERROR_CORRUPTED_FILE;
		}

		if(frReadUint16(&file->reader, &cmapData[0]) != FR_SUCCESS)
		{
			free(cmapData);
			return FR_ERROR_CORRUPTED_FILE;
		}
		cmapData[0] /= 2;

		if(frSkipBytes(&file->reader, 6) != FR_SUCCESS)
		{
			free(cmapData);
			return FR_ERROR_CORRUPTED_FILE;
		}

		for(uint16_t j = 0; j < cmapData[0]; ++j)
		{
			if(frReadUint16(&file->reader, &cmapData[j + 1]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}

		if(frSkipBytes(&file->reader, 2) != FR_SUCCESS)
		{
			free(cmapData);
			return FR_ERROR_CORRUPTED_FILE;
		}

		for(uint16_t j = 0; j < cmapData[0]; ++j)
		{
			if(frReadUint16(&file->reader, &cmapData[j + cmapData[0] + 1]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}

		for(uint16_t j = 0; j < cmapData[0]; ++j)
		{
			if(frReadUint16(&file->reader, &cmapData[j + 2 * cmapData[0] + 1]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}

		for(uint16_t j = 0; j < cmapData[0]; ++j)
		{
			if(frReadUint16(&file->reader, &cmapData[j + 3 * cmapData[0] + 1]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}

		const uint16_t glyphIndexCount = cmapDataSize / sizeof(cmapData[0]) - 2 - 4 * cmapData[0];
		cmapData[4 * cmapData[0] + 1] = glyphIndexCount;
		for(uint16_t j = 0; j < glyphIndexCount; ++j)
		{
			if(frReadUint16(&file->reader, &cmapData[j + 4 * cmapData[0] + 2]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}

		file->font->cmapData = cmapData;

		return FR_SUCCESS;
	}

	if(format == FR_CMAP_FORMAT_12)
	{
		file->font->cmapFormat = FR_CMAP_FORMAT_12;

		if(frSkipBytes(&file->reader, 10) != FR_SUCCESS)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}

		uint32_t numGroups;
		if(frReadUint32(&file->reader, &numGroups) != FR_SUCCESS)
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
			if(frReadUint32(&file->reader, &cmapData[1 + i]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}

			if(frReadUint32(&file->reader, &cmapData[1 + numGroups + i]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}

			if(frReadUint32(&file->reader, &cmapData[1 + 2 * numGroups + i]) != FR_SUCCESS)
			{
				free(cmapData);
				return FR_ERROR_CORRUPTED_FILE;
			}
		}

		file->font->cmapData = cmapData;

		return FR_SUCCESS;
	}

	return FR_ERROR_CORRUPTED_FILE;
}

FrResult frLoadFont(const char* const path, FrFont* const font)
{
	*font = (FrFont){0};
	FrFontFile fontFile = {0};
	fontFile.reader.file = fopen(path, "rb");
	if(!fontFile.reader.file)
	{
		return FR_ERROR_FILE_NOT_FOUND;
	}
	fontFile.font = font;

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

	if(frReadUint16(&fontFile.reader, &fontFile.font->glyphCount) != FR_SUCCESS)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	fontFile.contourInfoCapacity = fontFile.font->glyphCount * UINT32_C(4);
	fontFile.font->contourInfos = malloc(fontFile.contourInfoCapacity * sizeof(fontFile.font->contourInfos[0]));
	if(!fontFile.font->contourInfos)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	fontFile.font->points = malloc(fontFile.font->glyphCount * 100 * sizeof(fontFile.font->points[0]));
	if(!fontFile.font->points)
	{
		free(fontFile.font->contourInfos);
		fclose(fontFile.reader.file);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	// loca
	if(frMoveTo(&fontFile.reader, fontFile.locaOffset) != FR_SUCCESS)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	fontFile.offsets = malloc((fontFile.font->glyphCount + 1) * sizeof(fontFile.offsets[0]));
	if(!fontFile.offsets)
	{
		fclose(fontFile.reader.file);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	for(uint16_t i = 0; i <= fontFile.font->glyphCount; ++i)
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
		free(fontFile.font->points);
		free(fontFile.font->contourInfos);
		free(fontFile.offsets);
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	// cmap
	if(frParseMappings(&fontFile) != FR_SUCCESS)
	{
		free(fontFile.font->points);
		free(fontFile.font->contourInfos);

		free(fontFile.font->glyphPositions);
		free(fontFile.font->glyphOffsets);

		free(fontFile.offsets);
		fclose(fontFile.reader.file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	free(fontFile.offsets);

	fclose(fontFile.reader.file);

	return FR_SUCCESS;
}

FrResult frGetGlyphId(const FrFont* const font, const uint32_t characterCode, uint32_t* const glyphId)
{
	if(font->cmapFormat == FR_CMAP_FORMAT_4)
	{
		if(characterCode > UINT16_MAX)
		{
			*glyphId = 0;
			return EXIT_SUCCESS;
		}

		const uint16_t segCount = *(uint16_t*)font->cmapData;
		const uint16_t* const endCodes = (uint16_t*)font->cmapData + 1;
		const uint16_t* const startCodes = endCodes + segCount;
		const uint16_t* const idDeltas = startCodes + segCount;
		const uint16_t* const idRangeOffsets = idDeltas + segCount;
		const uint16_t glyphIndexCount = *(uint16_t*)(idRangeOffsets + segCount);

		const uint16_t character = (uint16_t)characterCode;

		uint16_t i;
		for(i = 0; i < segCount && character > endCodes[i]; ++i);
		if(i == segCount)
		{
			*glyphId = 0;
			return FR_SUCCESS;
		}

		if(character < startCodes[i])
		{
			*glyphId = 0;
			return FR_SUCCESS;
		}

		if(idRangeOffsets[i] == 0)
		{
			*glyphId = (character + idDeltas[i]) % (UINT16_MAX + UINT32_C(1));
			return FR_SUCCESS;
		}

		const uint16_t offset = idRangeOffsets[i] / 2 + (character - startCodes[i]);
		if(offset >= segCount + glyphIndexCount)
		{
			return FR_ERROR_CORRUPTED_FILE;
		}
		*glyphId = *(idRangeOffsets + i + offset + 1);
		if(*glyphId == 0)
		{
			return FR_SUCCESS;
		}
		*glyphId = (*glyphId + idDeltas[i]) % (UINT16_MAX + UINT32_C(1));

		return FR_SUCCESS;
	}

	if(font->cmapFormat == FR_CMAP_FORMAT_12)
	{
		const uint32_t numGroups = *(uint32_t*)font->cmapData;
		const uint32_t* const startCharCodes = (uint32_t*)font->cmapData + 1;
		const uint32_t* const endCharCodes = startCharCodes + numGroups;
		const uint32_t* const startGlyphCodes = endCharCodes + numGroups;

		for(uint32_t i = 0; i < numGroups; ++i)
		{
			if(characterCode >= startCharCodes[i] && characterCode <= endCharCodes[i])
			{
				*glyphId = startGlyphCodes[i] + characterCode - startCharCodes[i];
				return FR_SUCCESS;
			}
		}

		return FR_ERROR_CORRUPTED_FILE;
	}

	return FR_ERROR_CORRUPTED_FILE;
}

void frFreeFont(FrFont* const font)
{
	free(font->points);
	free(font->contourInfos);

	free(font->glyphPositions);
	free(font->glyphOffsets);

	free(font->cmapData);
}

#define FR_1BYTE  0x80
#define FR_2BYTES 0xe0
#define FR_3BYTES 0xf0

#define FR_3BITS 0x07
#define FR_4BITS 0x0f
#define FR_5BITS 0x1f
#define FR_6BITS 0x3f
#define FR_7BITS 0x7f

FrResult frNextCharacterCode(FrStringReader* const stringReader, uint32_t* const characterCode)
{
	if(stringReader->string >= stringReader->end)
	{
		*characterCode = 0;
		return FR_SUCCESS;
	}

	const uint8_t first = *stringReader->string;
	if(first < FR_1BYTE)
	{
		*characterCode = first & FR_7BITS;
		++stringReader->string;
		return FR_SUCCESS;
	}

	if(stringReader->string + 1 >= stringReader->end)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(first < FR_2BYTES)
	{
		*characterCode = first & FR_5BITS;
		*characterCode <<= 6;
		++stringReader->string;

		*characterCode |= *stringReader->string & FR_6BITS;
		++stringReader->string;

		return FR_SUCCESS;
	}

	if(stringReader->string + 2 >= stringReader->end)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(first < FR_3BYTES)
	{
		*characterCode = first & FR_4BITS;
		*characterCode <<= 6;
		++stringReader->string;

		*characterCode |= *stringReader->string & FR_6BITS;
		*characterCode <<= 6;
		++stringReader->string;

		*characterCode |= *stringReader->string & FR_6BITS;
		*characterCode <<= 6;
		++stringReader->string;

		return FR_SUCCESS;
	}

	if(stringReader->string + 3 >= stringReader->end)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	*characterCode = first & FR_3BITS;
	*characterCode <<= 6;
	++stringReader->string;

	*characterCode |= *stringReader->string & FR_6BITS;
	*characterCode <<= 6;
	++stringReader->string;

	*characterCode |= *stringReader->string & FR_6BITS;
	*characterCode <<= 6;
	++stringReader->string;

	*characterCode |= *stringReader->string & FR_6BITS;
	*characterCode <<= 6;
	++stringReader->string;

	return FR_SUCCESS;
}
