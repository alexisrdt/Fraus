#ifndef FRAUS_INFLATE_TEMP_H
#define FRAUS_INFLATE_TEMP_H

#include <stdint.h>

#include "../utils.h"

/*
 * Inflate deflate encoded data
 * - pData: deflate encoded input data
 * - size: number of bytes in the input data
 * - ppResult: buffer in which to store the result
 */
FrResult frInflate(const uint8_t* pData, size_t size, uint8_t** ppResult);

#endif
