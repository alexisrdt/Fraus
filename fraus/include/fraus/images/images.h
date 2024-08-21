#ifndef FRAUS_IMAGES_H
#define FRAUS_IMAGES_H

#include <stdint.h>

#include "./inflate.h"
#include "../utils.h"

// Image type and number of channels
typedef enum FrImageType
{
	FR_GRAY       = 1,
	FR_GRAY_ALPHA = 2,
	FR_RGB        = 3,
	FR_RGB_ALPHA  = 4
} FrImageType;

typedef struct FrImage
{
	uint32_t width;
	uint32_t height;
	uint8_t* data;
	FrImageType type;
} FrImage;

FrResult frLoadPNG(const char* path, FrImage* pImage);

#endif
