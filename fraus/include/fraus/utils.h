#ifndef FRAUS_UTILS_H
#define FRAUS_UTILS_H

#include <stddef.h>

/*
 * Result type returned by most Fraus functions.
 */
typedef enum FrResult
{
	FR_SUCCESS,
	FR_ERROR_FILE_NOT_FOUND,
	FR_ERROR_CORRUPTED_FILE,
	FR_ERROR_OUT_OF_HOST_MEMORY,
	FR_ERROR_OUT_OF_DEVICE_MEMORY,
	FR_ERROR_INVALID_ARGUMENT,
	FR_ERROR_UNKNOWN
} FrResult;

/*
 * Get the length of an array.
 * x has to be an array, not a pointer.
*/
#define FR_LEN(x) \
(sizeof(x) / sizeof(x[0]))

#define FR_SWAP_BYTES_U32(u) \
( \
	((u & 0x000000FF) << 0x18) | \
	((u & 0x0000FF00) << 0x08) | \
	((u & 0x00FF0000) >> 0x08) | \
	((u & 0xFF000000) >> 0x18) \
)

/*
 * A comparison function.
 *
 * Parameters:
 * - pFirstVoid: Pointer to the first object to compare.
 * - pSecondVoid: Pointer to the second object to compare.
 * 
 * Returns:
 * - A negative number if the first object is less than the second.
 * - A positive number if the first object is more than the second.
 * - 0 if both objects are equal.
 */
typedef int (*FrCompareFunction)(const void* pFirstVoid, const void* pSecondVoid);

/*
 * Merge two sorted arrays based on a comparison function.
 * If two elements compare equal, the element from the first array has priority.
 *
 * Parameters:
 * - firstCount: The number of elements in the first array.
 * - first: The first array.
 * - secondCount: The number of elements in the second array.
 * - second: The second array.
 * - final: The final array.
 * - elementSize: The size of each element.
 * - compare: The comparison function.
 */
void frMergeSorted(size_t firstCount, const void* first, size_t secondCount, const void* second, void* restrict final, size_t elementSize, FrCompareFunction compare);

#endif
