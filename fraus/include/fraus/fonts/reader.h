#ifndef FRAUS_FONTS_READER_H
#define FRAUS_FONTS_READER_H

#include <stdint.h>
#include <stdio.h>

#include "fraus/utils.h"

#define FR_MSBF_TO_I16(bytes) \
((int16_t)(((int16_t)((bytes)[0]) << 8) | (int16_t)((bytes)[1])))

#define FR_MSBF_TO_U16(bytes) \
((uint16_t)(((uint16_t)((bytes)[0]) << 8) | (uint16_t)((bytes)[1])))

#define FR_BYTES_TO_U32(b3, b2, b1, b0) \
((uint32_t)(((uint32_t)b3 << 24) | ((uint32_t)b2 << 16) | ((uint32_t)b1 << 8) | (uint32_t)b0))

#define FR_MSBF_TO_U32(bytes) FR_BYTES_TO_U32((bytes)[0], (bytes)[1], (bytes)[2], (bytes)[3])

#define FR_F2D14_DIV 16384.f // 2^14

#define FR_F2D14_INT(x) \
((int8_t)((int8_t)((x & 0x8000) ? -2 : 0) + (int8_t)((x & 0x4000) > 0)))

#define FR_F2D14_FRAC(x) \
((uint16_t)((x) & 0x3FFF))

#define FR_F2D14_FLOAT(x) \
(FR_F2D14_INT(x) + FR_F2D14_FRAC(x) / FR_F2D14_DIV)

#define FR_BIT(value, index) \
((value) >> (index) & 1)

typedef struct FrFileReader
{
	FILE* file;
	uint8_t buffer[4];
} FrFileReader;

FrResult frReadInt8(FrFileReader* reader, int8_t* value);
FrResult frReadUint8(FrFileReader* reader, uint8_t* value);
FrResult frReadUint16(FrFileReader* reader, uint16_t* value);
FrResult frReadInt16(FrFileReader* reader, int16_t* value);
FrResult frReadUint32(FrFileReader* reader, uint32_t* value);
FrResult frReadF2d14(FrFileReader* reader, float* value);

FrResult frSkipBytes(FrFileReader* reader, long bytes);

FrResult frMoveTo(FrFileReader* reader, long offset);

#endif
