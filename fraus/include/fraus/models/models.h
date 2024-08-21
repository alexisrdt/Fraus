#ifndef FRAUS_MODELS_MODELS_H
#define FRAUS_MODELS_MODELS_H

#include <stdint.h>

#include "../math.h"
#include "../utils.h"
#include "./map.h"

typedef struct FrModel
{
	FrVertex* vertices;
	uint32_t vertexCount;
	uint32_t* indexes;
	uint32_t indexCount;
} FrModel;

FrResult frLoadOBJ(const char* path, FrModel* pModel);

#endif
