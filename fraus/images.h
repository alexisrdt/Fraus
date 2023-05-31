#ifndef FRAUS_IMAGES_H
#define FRAUS_IMAGES_H

#include <stdint.h>

#include "utils.h"

typedef enum FrImageType
{
	FR_RGB,
	FR_RGBA
} FrImageType;

typedef struct FrImage
{
	uint32_t width;
	uint32_t height;
	uint8_t* data;
	FrImageType type;
} FrImage;

FrResult frLoadPng(const char* path, FrImage* pImage);

#endif
