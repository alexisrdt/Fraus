#ifndef FRAUS_FONTS_H
#define FRAUS_FONTS_H

#include "fraus/math.h"
#include "fraus/utils.h"
#include "fraus/fonts/reader.h"

#include <stdint.h>

typedef struct FrGlyphPos
{
	int16_t xMin;
	int16_t yMin;
	int16_t xMax;
	int16_t yMax;

	uint16_t advanceWidth;
	int16_t leftSideBearing;
} FrGlyphPos;

typedef enum FrCmapFormat
{
	FR_CMAP_FORMAT_4 = 4,
	FR_CMAP_FORMAT_12 = 12
} FrCmapFormat;

/*
 * Font struct:
 *
 * CPU
 * 
 * Positions:
 * - bounds of the glyph
 * - horizontal metrics
 * 
 * GPU
 * 
 * Offset:
 * - offset to the glyph contour info
 * - offset to the glyph points
 *
 * Contour info:
 * - number of contours
 * - number of points in each contour
 * 
 * Points:
 * - points in contours, in order, explicit and implied, on curve then off curve
 */
typedef struct FrFont
{
	uint16_t glyphCount;

	// CPU
	FrGlyphPos* glyphPositions;
	FrCmapFormat cmapFormat;
	void* cmapData;

	// GPU
	uint32_t* glyphOffsets;
	uint16_t contourInfoCount;
	uint32_t* contourInfos;
	uint32_t pointCount;
	FrVec2* points;
} FrFont;

FrResult frLoadFont(const char* path, FrFont* pFont);
FrResult frGetGlyphId(const FrFont* pFont, uint32_t characterCode, uint32_t* pGlyphId);
FrResult frFreeFont(FrFont* pFont);

typedef struct FrStringReader
{
	const char* string;
	const char* const end;
} FrStringReader;

/*
 * Get the next character code in an UTF-8 string.
 *
 * Parameters:
 * - pStringReader: A valid pointer to a string reader.
 * - pCharacterCode: A valid pointer to a 32-bit unsigned integer that will receive the value of the character code.
 * 
 * Returns:
 * - FR_SUCCESS if all went well.
 * - FR_ERROR_INVALID_ARGUMENT if one of the parameters or the string is NULL, or if the sequence of chars does not make sense.
 */
FrResult frNextCharacterCode(FrStringReader* pStringReader, uint32_t* pCharacterCode);

#endif
