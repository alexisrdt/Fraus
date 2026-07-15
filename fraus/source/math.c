#include "../include/fraus/math.h"

#include <math.h>

float frDot(const FrVec3* const first, const FrVec3* const second)
{
	return first->x * second->x + first->y * second->y + first->z * second->z;
}

void frNormalize(FrVec3* const vector)
{
	const float magnitude = sqrtf(frDot(vector, vector));

	vector->x /= magnitude;
	vector->y /= magnitude;
	vector->z /= magnitude;
}

FrVec3 frCross(const FrVec3* const first, const FrVec3* const second)
{
	return (FrVec3){
		.x = first->y * second->z - first->z * second->y,
		.y = first->z * second->x - first->x * second->z,
		.z = first->x * second->y - first->y * second->x
	};
}

FrVec3 frScale(const FrVec3* const vector, const float scalar)
{
	return (FrVec3){
		.x = vector->x * scalar,
		.y = vector->y * scalar,
		.z = vector->z * scalar
	};
}

FrVec3 frAdd(const FrVec3* const first, const FrVec3* const second)
{
	return (FrVec3) {
		.x = first->x + second->x,
		.y = first->y + second->y,
		.z = first->z + second->z
	};
}

FrVec3 frSubstract(const FrVec3* const first, const FrVec3* const second)
{
	return (FrVec3){
		.x = first->x - second->x,
		.y = first->y - second->y,
		.z = first->z - second->z
	};
}

void frIdentity(float matrix[const 16])
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

void frTranslation(float matrix[const 16], const float x, const float y, const float z)
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

void frZRotation(float matrix[const 16], const float angle)
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

void frScaling(float matrix[const 16], const float x, const float y, const float z)
{
	matrix[ 0] =   x;
	matrix[ 1] = 0.f;
	matrix[ 2] = 0.f;
	matrix[ 3] = 0.f;
	matrix[ 4] = 0.f;
	matrix[ 5] =   y;
	matrix[ 6] = 0.f;
	matrix[ 7] = 0.f;
	matrix[ 8] = 0.f;
	matrix[ 9] = 0.f;
	matrix[10] =   z;
	matrix[11] = 0.f;
	matrix[12] = 0.f;
	matrix[13] = 0.f;
	matrix[14] = 0.f;
	matrix[15] = 1.f;

}

void frLookDir(float matrix[const 16], const FrVec3* const eye, const FrVec3* const forward, const FrVec3* const right, const FrVec3* const up)
{
	matrix[ 0] = right->x;
	matrix[ 1] = forward->x;
	matrix[ 2] = up->x;
	matrix[ 3] = 0.f;
	matrix[ 4] = right->y;
	matrix[ 5] = forward->y;
	matrix[ 6] = up->y;
	matrix[ 7] = 0.f;
	matrix[ 8] = right->z;
	matrix[ 9] = forward->z;
	matrix[10] = up->z;
	matrix[11] = 0.f;
	matrix[12] = -frDot(eye, right);
	matrix[13] = -frDot(eye, forward);
	matrix[14] = -frDot(eye, up);
	matrix[15] = 1.f;
}

void frLookAt(float matrix[const 16], const FrVec3* const eye, const FrVec3* const objective)
{
	const FrVec3 worldUp = {
		.x = 0.f,
		.y = 0.f,
		.z = 1.f
	};

	FrVec3 forward = frSubstract(objective, eye);
	frNormalize(&forward);

	FrVec3 right = frCross(&forward, &worldUp);
	frNormalize(&right);

	const FrVec3 up = frCross(&right, &forward);
	// No need to normalize, forward and right are orthonormal

	frLookDir(matrix, eye, &forward, &right, &up);
}

void frPerspective(float matrix[const 16], const float fov, const float aspect, const float near, const float far)
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

void frPerspectiveInfiniteFar(float matrix[const 16], const float fov, const float aspect, const float near)
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

void frMultiply(const float* restrict const first, const float* restrict const second, float* restrict const result)
{
	result[ 0] = first[ 0] * second[ 0] + first[ 1] * second[ 4] + first[ 2] * second[ 8] + first[ 3] * second[12];
	result[ 1] = first[ 0] * second[ 1] + first[ 1] * second[ 5] + first[ 2] * second[ 9] + first[ 3] * second[13];
	result[ 2] = first[ 0] * second[ 2] + first[ 1] * second[ 6] + first[ 2] * second[10] + first[ 3] * second[14];
	result[ 3] = first[ 0] * second[ 3] + first[ 1] * second[ 7] + first[ 2] * second[11] + first[ 3] * second[15];
	result[ 4] = first[ 4] * second[ 0] + first[ 5] * second[ 4] + first[ 6] * second[ 8] + first[ 7] * second[12];
	result[ 5] = first[ 4] * second[ 1] + first[ 5] * second[ 5] + first[ 6] * second[ 9] + first[ 7] * second[13];
	result[ 6] = first[ 4] * second[ 2] + first[ 5] * second[ 6] + first[ 6] * second[10] + first[ 7] * second[14];
	result[ 7] = first[ 4] * second[ 3] + first[ 5] * second[ 7] + first[ 6] * second[11] + first[ 7] * second[15];
	result[ 8] = first[ 8] * second[ 0] + first[ 9] * second[ 4] + first[10] * second[ 8] + first[11] * second[12];
	result[ 9] = first[ 8] * second[ 1] + first[ 9] * second[ 5] + first[10] * second[ 9] + first[11] * second[13];
	result[10] = first[ 8] * second[ 2] + first[ 9] * second[ 6] + first[10] * second[10] + first[11] * second[14];
	result[11] = first[ 8] * second[ 3] + first[ 9] * second[ 7] + first[10] * second[11] + first[11] * second[15];
	result[12] = first[12] * second[ 0] + first[13] * second[ 4] + first[14] * second[ 8] + first[15] * second[12];
	result[13] = first[12] * second[ 1] + first[13] * second[ 5] + first[14] * second[ 9] + first[15] * second[13];
	result[14] = first[12] * second[ 2] + first[13] * second[ 6] + first[14] * second[10] + first[15] * second[14];
	result[15] = first[12] * second[ 3] + first[13] * second[ 7] + first[14] * second[11] + first[15] * second[15];
}
