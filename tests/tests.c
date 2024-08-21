#include <assert.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>

#include <fraus/fraus.h>

int compareInts(const void* pFirstVoid, const void* pSecondVoid)
{
	const int* const pFirst = pFirstVoid;
	const int* const pSecond = pSecondVoid;

	return (*pFirst > *pSecond) - (*pFirst < *pSecond);
}

typedef struct FrSortTest
{
	int a;
	int b;
} FrSortTest;

int compareSortTests(const void* pFirstVoid, const void* pSecondVoid)
{
	const FrSortTest* const pFirst = pFirstVoid;
	const FrSortTest* const pSecond = pSecondVoid;

	return (pFirst->a > pSecond->a) - (pFirst->a < pSecond->a);
}

typedef struct FrF2d14
{
	uint16_t hex;
	int8_t integer;
	uint16_t fraction;
	float floating;
} FrF2d14;

#define FR_FATAL(...) \
fprintf(stderr, "[FRAUS|FATAL]\n\terrno %d: %s\n\tFraus: ", errno, strerror(errno)); \
fprintf(stderr, __VA_ARGS__); \
fprintf(stderr, "\n"); \
return EXIT_FAILURE;

int main(void)
{
	// Test 1: frMergeSorted
	const int first[] = {1, 2, 6, 7, 9};
	const int second[] = {3, 4, 5, 8, 10, 11, 12};
	int final[FR_LEN(first) + FR_LEN(second)];
	const int solution[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
	static_assert(FR_LEN(final) == FR_LEN(solution), "final and solution should have the same length");
	frMergeSorted(FR_LEN(first), first, FR_LEN(second), second, final, sizeof(final[0]), compareInts);
	if(memcmp(final, solution, sizeof(final)) != 0)
	{
		FR_FATAL("First frMergeSorted test failed.");
	}

	const FrSortTest first2[] = {
		{1, -7},
		{3, 3}
	};
	const FrSortTest second2[] = {
		{2, -47},
		{3, 4}
	};
	FrSortTest final2[FR_LEN(first2) + FR_LEN(second2)];
	const FrSortTest solution2[] = {
		first2[0],
		second2[0],
		first2[1],
		second2[1]
	};
	static_assert(FR_LEN(final2) == FR_LEN(solution2), "final2 and solution2 should have the same length");
	frMergeSorted(FR_LEN(first2), first2, FR_LEN(second2), second2, final2, sizeof(final2[0]), compareSortTests);
	if(memcmp(final2, solution2, sizeof(final2)) != 0)
	{
		FR_FATAL("Second frMergeSorted test failed.");
	}

	// Test 2: FrF2d14
	const FrF2d14 arr[] = {
		{0x7FFF, 1, 16383, 1.999939f},
		{0x7000, 1, 12288, 1.75f},
		{0x0001, 0, 1, 0.000061f},
		{0x0000, 0, 0, 0.f},
		{0xFFFF, -1, 16383, -0.000061f},
		{0x8000, -2, 0, -2.f}
	};
	const size_t arrLen = FR_LEN(arr);
	for(size_t i = 0; i < arrLen; ++i)
	{
		const int8_t integer = FR_F2D14_INT(arr[i].hex);
		if(integer != arr[i].integer)
		{
			FR_FATAL("Failure #%zu: FR_F2D14_INT(0x%"PRIX16") = %"PRId8", expected %"PRId8"", i, arr[i].hex, integer, arr[i].integer);
		}

		const uint16_t fraction = FR_F2D14_FRAC(arr[i].hex);
		if(fraction != arr[i].fraction)
		{
			FR_FATAL("Failure #%zu: FR_F2D14_FRAC(0x%"PRIX16") = %"PRIu16", expected %"PRIu16"", i, arr[i].hex, fraction, arr[i].fraction);
		}

		const float floating = FR_F2D14_FLOAT(arr[i].hex);
		if(fabs(floating - arr[i].floating) > FLT_EPSILON)
		{
			FR_FATAL("Failure #%zu: FR_F2D14_FLOAT(0x%"PRIX16") = %f, expected %f", i, arr[i].hex, floating, arr[i].floating);
		}
	}

	return EXIT_SUCCESS;
}
