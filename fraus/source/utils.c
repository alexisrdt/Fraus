#include "../../fraus/include/fraus/utils.h"

#include <string.h>

void frMergeSorted(size_t firstCount, const void* first, size_t secondCount, const void* second, void* restrict final, size_t elementSize, FrCompareFunction compare)
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
