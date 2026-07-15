#include "../../fraus/include/fraus/utils.h"

#include <string.h>

void frMergeSorted(const size_t firstCount, const void* const first, const size_t secondCount, const void* const second, void* restrict const final, const size_t elementSize, const FrCompareFunction compare)
{
	const char* firstChar = first;
	const char* secondChar = second;
	char* restrict finalChar = final;

	const char* const firstCharEnd = firstChar + firstCount * elementSize;
	const char* const secondCharEnd = secondChar + secondCount * elementSize;

	while(firstChar < firstCharEnd && secondChar < secondCharEnd)
	{
		if(compare(firstChar, secondChar) <= 0)
		{
			memcpy(finalChar, firstChar, elementSize);
			firstChar += elementSize;
		}
		else
		{
			memcpy(finalChar, secondChar, elementSize);
			secondChar += elementSize;
		}
		finalChar += elementSize;
	}

	while(firstChar < firstCharEnd)
	{
		memcpy(finalChar, firstChar, elementSize);
		firstChar += elementSize;
		finalChar += elementSize;
	}

	while(secondChar < secondCharEnd)
	{
		memcpy(finalChar, secondChar, elementSize);
		secondChar += elementSize;
		finalChar += elementSize;
	}
}
