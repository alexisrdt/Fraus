#include "math.h"

#include <math.h>

float frDot(const FrVec3* pFirst, const FrVec3* pSecond)
{
	return pFirst->x * pSecond->x + pFirst->y * pSecond->y + pFirst->z * pSecond->z;
}

void frNormalize(FrVec3* pVector)
{
	const float magnitude = sqrtf(frDot(pVector, pVector));

	pVector->x /= magnitude;
	pVector->y /= magnitude;
	pVector->z /= magnitude;
}

FrVec3 frCross(const FrVec3* pFirst, const FrVec3* pSecond)
{
	return (FrVec3){
		.x = pFirst->y * pSecond->z - pFirst->z * pSecond->y,
		.y = pFirst->z * pSecond->x - pFirst->x * pSecond->z,
		.z = pFirst->x * pSecond->y - pFirst->y * pSecond->x
	};
}

FrVec3 frScale(const FrVec3* pVector, float scalar)
{
	return (FrVec3){
		.x = pVector->x * scalar,
		.y = pVector->y * scalar,
		.z = pVector->z * scalar
	};
}

FrVec3 frAdd(const FrVec3* pFirst, const FrVec3* pSecond)
{
	return (FrVec3) {
		.x = pFirst->x + pSecond->x,
		.y = pFirst->y + pSecond->y,
		.z = pFirst->z + pSecond->z
	};
}

FrVec3 frSubstract(const FrVec3* pFirst, const FrVec3* pSecond)
{
	return (FrVec3){
		.x = pFirst->x - pSecond->x,
		.y = pFirst->y - pSecond->y,
		.z = pFirst->z - pSecond->z
	};
}

void frIdentity(float matrix[16])
{
	matrix[ 0] = 1.f;
	matrix[ 1] = 0.f;
	matrix[ 2] = 0.f;
	matrix[ 3] = 0.f;
	matrix[ 4] = 0.f;
	matrix[ 5] = 1.f;
	matrix[ 6] = 0.f;
	matrix[ 7] = 0.f;
	matrix[ 8] = 0.f;
	matrix[ 9] = 0.f;
	matrix[10] = 1.f;
	matrix[11] = 0.f;
	matrix[12] = 0.f;
	matrix[13] = 0.f;
	matrix[14] = 0.f;
	matrix[15] = 1.f;
}

void frTranslation(float matrix[16], float x, float y, float z)
{
	matrix[ 0] = 1.f;
	matrix[ 1] = 0.f;
	matrix[ 2] = 0.f;
	matrix[ 3] = 0.f;
	matrix[ 4] = 0.f;
	matrix[ 5] = 1.f;
	matrix[ 6] = 0.f;
	matrix[ 7] = 0.f;
	matrix[ 8] = 0.f;
	matrix[ 9] = 0.f;
	matrix[10] = 1.f;
	matrix[11] = 0.f;
	matrix[12] =   x;
	matrix[13] =   y;
	matrix[14] =   z;
	matrix[15] = 1.f;
}

void frZRotation(float matrix[16], float angle)
{
	const float c = cosf(angle);
	const float s = sinf(angle);

	matrix[ 0] =   c;
	matrix[ 1] =   s;
	matrix[ 2] = 0.f;
	matrix[ 3] = 0.f;
	matrix[ 4] =  -s;
	matrix[ 5] =   c;
	matrix[ 6] = 0.f;
	matrix[ 7] = 0.f;
	matrix[ 8] = 0.f;
	matrix[ 9] = 0.f;
	matrix[10] = 1.f;
	matrix[11] = 0.f;
	matrix[12] = 0.f;
	matrix[13] = 0.f;
	matrix[14] = 0.f;
	matrix[15] = 1.f;
}

void frLookDir(float matrix[16], const FrVec3* pEye, const FrVec3* pForward, const FrVec3* pRight, const FrVec3* pUp)
{
	matrix[ 0] = pRight->x;
	matrix[ 1] = pForward->x;
	matrix[ 2] = pUp->x;
	matrix[ 3] = 0.f;
	matrix[ 4] = pRight->y;
	matrix[ 5] = pForward->y;
	matrix[ 6] = pUp->y;
	matrix[ 7] = 0.f;
	matrix[ 8] = pRight->z;
	matrix[ 9] = pForward->z;
	matrix[10] = pUp->z;
	matrix[11] = 0.f;
	matrix[12] = -frDot(pEye, pRight);
	matrix[13] = -frDot(pEye, pForward);
	matrix[14] = -frDot(pEye, pUp);
	matrix[15] = 1.f;
}

void frLookAt(float matrix[16], const FrVec3* pEye, const FrVec3* pObjective)
{
	static const FrVec3 worldUp = {
		.x = 0.f,
		.y = 0.f,
		.z = 1.f
	};

	FrVec3 forward = frSubstract(pObjective, pEye);
	frNormalize(&forward);

	FrVec3 right = frCross(&forward, &worldUp);
	frNormalize(&right);

	const FrVec3 up = frCross(&right, &forward);
	// No need to normalize, forward and right are orthonormal

	frLookDir(matrix, pEye, &forward, &right, &up);
}

void frPerspective(float matrix[16], float fov, float aspect, float near, float far)
{
	const float inv = 1 / tanf(fov / 2.f);
	const float frac = far / (far - near);

	matrix[ 0] = inv / aspect;
	matrix[ 1] = 0.f;
	matrix[ 2] = 0.f;
	matrix[ 3] = 0.f;
	matrix[ 4] = 0.f;
	matrix[ 5] = 0.f;
	matrix[ 6] = frac;
	matrix[ 7] = 1.f;
	matrix[ 8] = 0.f;
	matrix[ 9] = -inv;
	matrix[10] = 0;
	matrix[11] = 0;
	matrix[12] = 0;
	matrix[13] = 0;
	matrix[14] = -near * frac;
	matrix[15] = 0.f;
}

void frPerspectiveInfiniteFar(float matrix[16], float fov, float aspect, float near)
{
	const float inv = 1 / tanf(fov / 2.f);

	matrix[0] = inv / aspect;
	matrix[1] = 0.f;
	matrix[2] = 0.f;
	matrix[3] = 0.f;
	matrix[4] = 0.f;
	matrix[5] = 0.f;
	matrix[6] = 1.f;
	matrix[7] = 1.f;
	matrix[8] = 0.f;
	matrix[9] = -inv;
	matrix[10] = 0;
	matrix[11] = 0;
	matrix[12] = 0;
	matrix[13] = 0;
	matrix[14] = -near;
	matrix[15] = 0.f;
}

void frMultiply(const float* restrict pFirst, const float* restrict pSecond, float* restrict pResult)
{
	pResult[ 0] = pFirst[ 0] * pSecond[ 0] + pFirst[ 1] * pSecond[ 4] + pFirst[ 2] * pSecond[ 8] + pFirst[ 3] * pSecond[12];
	pResult[ 1] = pFirst[ 0] * pSecond[ 1] + pFirst[ 1] * pSecond[ 5] + pFirst[ 2] * pSecond[ 9] + pFirst[ 3] * pSecond[13];
	pResult[ 2] = pFirst[ 0] * pSecond[ 2] + pFirst[ 1] * pSecond[ 6] + pFirst[ 2] * pSecond[10] + pFirst[ 3] * pSecond[14];
	pResult[ 3] = pFirst[ 0] * pSecond[ 3] + pFirst[ 1] * pSecond[ 7] + pFirst[ 2] * pSecond[11] + pFirst[ 3] * pSecond[15];
	pResult[ 4] = pFirst[ 4] * pSecond[ 0] + pFirst[ 5] * pSecond[ 4] + pFirst[ 6] * pSecond[ 8] + pFirst[ 7] * pSecond[12];
	pResult[ 5] = pFirst[ 4] * pSecond[ 1] + pFirst[ 5] * pSecond[ 5] + pFirst[ 6] * pSecond[ 9] + pFirst[ 7] * pSecond[13];
	pResult[ 6] = pFirst[ 4] * pSecond[ 2] + pFirst[ 5] * pSecond[ 6] + pFirst[ 6] * pSecond[10] + pFirst[ 7] * pSecond[14];
	pResult[ 7] = pFirst[ 4] * pSecond[ 3] + pFirst[ 5] * pSecond[ 7] + pFirst[ 6] * pSecond[11] + pFirst[ 7] * pSecond[15];
	pResult[ 8] = pFirst[ 8] * pSecond[ 0] + pFirst[ 9] * pSecond[ 4] + pFirst[10] * pSecond[ 8] + pFirst[11] * pSecond[12];
	pResult[ 9] = pFirst[ 8] * pSecond[ 1] + pFirst[ 9] * pSecond[ 5] + pFirst[10] * pSecond[ 9] + pFirst[11] * pSecond[13];
	pResult[10] = pFirst[ 8] * pSecond[ 2] + pFirst[ 9] * pSecond[ 6] + pFirst[10] * pSecond[10] + pFirst[11] * pSecond[14];
	pResult[11] = pFirst[ 8] * pSecond[ 3] + pFirst[ 9] * pSecond[ 7] + pFirst[10] * pSecond[11] + pFirst[11] * pSecond[15];
	pResult[12] = pFirst[12] * pSecond[ 0] + pFirst[13] * pSecond[ 4] + pFirst[14] * pSecond[ 8] + pFirst[15] * pSecond[12];
	pResult[13] = pFirst[12] * pSecond[ 1] + pFirst[13] * pSecond[ 5] + pFirst[14] * pSecond[ 9] + pFirst[15] * pSecond[13];
	pResult[14] = pFirst[12] * pSecond[ 2] + pFirst[13] * pSecond[ 6] + pFirst[14] * pSecond[10] + pFirst[15] * pSecond[14];
	pResult[15] = pFirst[12] * pSecond[ 3] + pFirst[13] * pSecond[ 7] + pFirst[14] * pSecond[11] + pFirst[15] * pSecond[15];
}
