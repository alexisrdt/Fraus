#ifndef FRAUS_INFLATE_TEMP_H
#define FRAUS_INFLATE_TEMP_H

#include <stddef.h>
#include <stdint.h>

#include "../utils.h"

/*
 * Inflate deflate encoded data
 * - data: deflate encoded input data
 * - size: number of bytes in the input data
 * - result: buffer in which to store the result
 */
FrResult frInflate(const uint8_t* data, size_t size, uint8_t* result);

#endif
