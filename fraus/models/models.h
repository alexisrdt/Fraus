#ifndef FRAUS_MODELS_MODELS_H
#define FRAUS_MODELS_MODELS_H

#include <stdint.h>

#include "../math.h"
#include "../utils.h"
#include "map.h"

typedef struct FrModel
{
	FrVertex* pVertices;
	uint32_t vertexCount;
	uint32_t* pIndexes;
	uint32_t indexCount;
} FrModel;

FrResult frLoadOBJ(const char* pPath, FrModel* pModel);

#endif
