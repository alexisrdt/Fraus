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

void frLookAt(float matrix[16], const FrVec3* pEye, const FrVec3* pObject)
{
	const FrVec3 worldUp = {
		.x = 0.f,
		.y = 0.f,
		.z = 1.f
	};

	FrVec3 forward = frSubstract(pObject, pEye);
	frNormalize(&forward);

	FrVec3 right = frCross(&forward, &worldUp);
	frNormalize(&right);

	FrVec3 up = frCross(&right, &forward);
	frNormalize(&up);

	matrix[ 0] = right.x;
	matrix[ 1] = forward.x;
	matrix[ 2] = up.x;
	matrix[ 3] = 0.f;
	matrix[ 4] = right.y;
	matrix[ 5] = forward.y;
	matrix[ 6] = up.y;
	matrix[ 7] = 0.f;
	matrix[ 8] = right.z;
	matrix[ 9] = forward.z;
	matrix[10] = up.z;
	matrix[11] = 0.f;
	matrix[12] = -frDot(pEye, &right);
	matrix[13] = -frDot(pEye, &forward);
	matrix[14] = -frDot(pEye, &up);
	matrix[15] = 1.f;
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
