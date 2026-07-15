#ifndef FRAUS_MATH_H
#define FRAUS_MATH_H

#include <stdint.h>

#define PI 3.14159265358979323846f

typedef union FrVec2
{
	struct {
		float x;
		float y;
	};
	struct {
		float u;
		float v;
	};
} FrVec2;

typedef union FrVec3
{
	struct {
		float x;
		float y;
		float z;
	};
	struct {
		float r;
		float g;
		float b;
	};
} FrVec3;

typedef struct FrVertex
{
	FrVec3 position;
	FrVec2 textureCoordinates;
	FrVec3 normal;
} FrVertex;

float frDot(const FrVec3* first, const FrVec3* second);
void frNormalize(FrVec3* vector);
FrVec3 frCross(const FrVec3* first, const FrVec3* second);
FrVec3 frScale(const FrVec3* vector, float scalar);
FrVec3 frAdd(const FrVec3* first, const FrVec3* second);
FrVec3 frSubstract(const FrVec3* first, const FrVec3* second);

void frIdentity(float matrix[16]);
void frTranslation(float matrix[16], float x, float y, float z);
void frZRotation(float matrix[16], float angle);
void frScaling(float matrix[16], float x, float y, float z);

void frLookDir(float matrix[16], const FrVec3* eye, const FrVec3* forward, const FrVec3* right, const FrVec3* up);
void frLookAt(float matrix[16], const FrVec3* eye, const FrVec3* objective);
void frPerspective(float matrix[16], float fov, float aspect, float near, float far);
void frPerspectiveInfiniteFar(float matrix[16], float fov, float aspect, float near);

void frMultiply(const float* restrict first, const float* restrict second, float* restrict result);

#endif
