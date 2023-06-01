#ifndef FRAUS_IMAGES_H
#define FRAUS_IMAGES_H

#include <stdint.h>

#include "utils.h"

typedef enum FrImageType
{
	FR_GRAY,
	FR_GRAY_ALPHA,
	FR_RGB,
	FR_RGB_ALPHA
} FrImageType;

typedef struct FrImage
{
	uint32_t width;
	uint32_t height;
	uint8_t* data;
	FrImageType type;
} FrImage;

FrResult frLoadPNG(const char* pPath, FrImage* pImage);

#endif
