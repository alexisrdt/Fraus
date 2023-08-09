#include "map.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

FrResult frCreateMap(uint32_t size, FrMap* pMap)
{
	pMap->ppNodes = calloc(size, sizeof(FrMapNode*));
	if(!pMap->ppNodes) return FR_ERROR_OUT_OF_MEMORY;

	pMap->size = size;

	return FR_SUCCESS;
}

static inline uint32_t frHash(const FrVertex* pVertex)
{
	uint32_t hash = 65521;

	hash ^= *(uint32_t*)&pVertex->position.x * 2;
	hash ^= *(uint32_t*)&pVertex->position.y * 3;
	hash ^= *(uint32_t*)&pVertex->position.z * 5;
	hash ^= *(uint32_t*)&pVertex->textureCoordinates.u * 7;
	hash ^= *(uint32_t*)&pVertex->textureCoordinates.v * 11;
	hash ^= *(uint32_t*)&pVertex->normal.x * 13;
	hash ^= *(uint32_t*)&pVertex->normal.y * 17;
	hash ^= *(uint32_t*)&pVertex->normal.z * 19;

	return hash;
}

static inline bool frCompareVertices(const FrVertex* pFirst, const FrVertex* pSecond)
{
	return memcmp(pFirst, pSecond, sizeof(FrVertex)) == 0;
}

FrResult frGetOrInsertMap(FrMap* pMap, const FrVertex* pKey, uint32_t newValue, uint32_t* pValue)
{
	const uint32_t hash = frHash(pKey);
	const uint32_t index = hash % pMap->size;

	FrMapNode* pNode = pMap->ppNodes[index];
	while(pNode)
	{
		if(frCompareVertices(&pNode->key, pKey))
		{
			*pValue = pNode->value;
			return FR_SUCCESS;
		}

		pNode = pNode->pNext;
	}

	FrMapNode* pNewNode = malloc(sizeof(FrMapNode));
	if(!pNewNode) return FR_ERROR_OUT_OF_MEMORY;

	pNewNode->key = *pKey;
	pNewNode->value = newValue;
	pNewNode->pNext = pMap->ppNodes[index];

	pMap->ppNodes[index] = pNewNode;

	*pValue = newValue;

	return FR_SUCCESS;
}

void frDestroyMap(FrMap* pMap)
{
	FrMapNode* pNode;
	FrMapNode* pNext;
	
	while(pMap->size--)
	{
		pNode = pMap->ppNodes[pMap->size];
		while(pNode)
		{
			pNext = pNode->pNext;
			free(pNode);
			pNode = pNext;
		}
	}

	free(pMap->ppNodes);
}
