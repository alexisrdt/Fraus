#ifndef FRAUS_VECTOR_H
#define FRAUS_VECTOR_H

#include "./utils.h"

#define FR_DECLARE_VECTOR(type, name) \
typedef struct Fr##name##Vector \
{ \
	size_t size; \
	size_t capacity; \
	type* pData; \
} Fr##name##Vector; \
\
FrResult frCreate##name##Vector(Fr##name##Vector* pVector); \
FrResult frCreateAndReserve##name##Vector(Fr##name##Vector* pVector, size_t capacity); \
void frDestroy##name##Vector(Fr##name##Vector* pVector); \
FrResult frPushBack##name##Vector(Fr##name##Vector* pVector, type value);

#define FR_DEFINE_VECTOR(type, name) \
FrResult frCreate##name##Vector(Fr##name##Vector* pVector) \
{ \
	pVector->size = 0; \
	pVector->capacity = 0; \
	pVector->pData = NULL; \
\
	return FR_SUCCESS; \
} \
\
FrResult frCreateAndReserve##name##Vector(Fr##name##Vector* pVector, size_t capacity) \
{ \
	pVector->size = 0; \
	pVector->capacity = capacity; \
	pVector->pData = malloc(capacity * sizeof(type)); \
	if(!pVector->pData) return FR_ERROR_OUT_OF_MEMORY; \
\
	return FR_SUCCESS; \
} \
\
void frDestroy##name##Vector(Fr##name##Vector* pVector) \
{ \
	free(pVector->pData); \
} \
\
FrResult frPushBack##name##Vector(Fr##name##Vector* pVector, type value) \
{ \
	if(pVector->size == pVector->capacity) \
	{ \
		size_t newCapacity = pVector->capacity == 0 ? 1 : pVector->capacity * 2; \
\
		type* pNewData = realloc(pVector->pData, newCapacity * sizeof(type)); \
		if(!pNewData) return FR_ERROR_OUT_OF_MEMORY; \
\
		pVector->pData = pNewData; \
		pVector->capacity = newCapacity; \
	} \
\
	pVector->pData[pVector->size++] = value; \
\
	return FR_SUCCESS; \
}

#endif
