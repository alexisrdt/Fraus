#ifndef FRAUS_MODELS_SET_H
#define FRAUS_MODELS_SET_H

#include <stdint.h>

#include "../math.h"
#include "../utils.h"

typedef struct FrMapNode
{
	FrVertex key;
	uint32_t value;
	struct FrMapNode* next;
} FrMapNode;

typedef struct FrMap
{
	FrMapNode** nodes;
	uint32_t size;
} FrMap;

FrResult frCreateMap(uint32_t size, FrMap* map);
FrResult frGetOrInsertMap(FrMap* map, const FrVertex* key, uint32_t newValue, uint32_t* value);
void frDestroyMap(FrMap* map);

#endif
