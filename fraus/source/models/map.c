#include "../../include/fraus/models/map.h"

#include <stdlib.h>
#include <string.h>

FrResult frCreateMap(const uint32_t size, FrMap* const map)
{
	map->nodes = calloc(size, sizeof(map->nodes[0]));
	if(!map->nodes)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	map->size = size;

	return FR_SUCCESS;
}

typedef union FrFloat
{
	float value;
	uint32_t bits;
} FrFloat;

static inline uint32_t frHash(const FrVertex* const vertex)
{
	uint32_t hash = 65521;

	const FrFloat positionX = {.value = vertex->position.x};
	const FrFloat positionY = {.value = vertex->position.y};
	const FrFloat positionZ = {.value = vertex->position.z};
	const FrFloat textureU = {.value = vertex->textureCoordinates.u};
	const FrFloat textureV = {.value = vertex->textureCoordinates.v};
	const FrFloat normalX = {.value = vertex->normal.x};
	const FrFloat normalY = {.value = vertex->normal.y};
	const FrFloat normalZ = {.value = vertex->normal.z};

	hash ^= positionX.bits * 2;
	hash ^= positionY.bits * 3;
	hash ^= positionZ.bits * 5;
	hash ^= textureU.bits * 7;
	hash ^= textureV.bits * 11;
	hash ^= normalX.bits * 13;
	hash ^= normalY.bits * 17;
	hash ^= normalZ.bits * 19;

	return hash;
}

static inline bool frCompareVertices(const FrVertex* const first, const FrVertex* const second)
{
	return memcmp(first, second, sizeof(*first)) == 0;
}

FrResult frGetOrInsertMap(FrMap* const map, const FrVertex* const key, const uint32_t newValue, uint32_t* const value)
{
	const uint32_t hash = frHash(key);
	const uint32_t index = hash % map->size;

	FrMapNode* node = map->nodes[index];
	while(node)
	{
		if(frCompareVertices(&node->key, key))
		{
			*value = node->value;
			return FR_SUCCESS;
		}

		node = node->next;
	}

	FrMapNode* const newNode = malloc(sizeof(*newNode));
	if(!newNode)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	newNode->key = *key;
	newNode->value = newValue;
	newNode->next = map->nodes[index];

	map->nodes[index] = newNode;

	*value = newValue;

	return FR_SUCCESS;
}

void frDestroyMap(FrMap* const map)
{
	FrMapNode* node;
	FrMapNode* next;
	
	while(map->size--)
	{
		node = map->nodes[map->size];
		while(node)
		{
			next = node->next;
			free(node);
			node = next;
		}
	}

	free(map->nodes);
}
