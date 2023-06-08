#ifndef FRAUS_INFLATE_TEMP_H
#define FRAUS_INFLATE_TEMP_H

#include <stdint.h>

#include "utils.h"

FrResult frInflate(const uint8_t* pData, size_t size, uint8_t** ppResult);

#endif
