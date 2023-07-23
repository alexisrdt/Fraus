#ifndef FRAUS_MATH_H
#define FRAUS_MATH_H

typedef struct FrVec3
{
	float x;
	float y;
	float z;
} FrVec3;

float frDot(const FrVec3* pFirst, const FrVec3* pSecond);
void frNormalize(FrVec3* pVector);
FrVec3 frCross(const FrVec3* pFirst, const FrVec3* pSecond);
FrVec3 frSubstract(const FrVec3* pFirst, const FrVec3* pSecond);

void frZRotation(float matrix[16], float angle);
void frLookAt(float matrix[16], const FrVec3* pEye, const FrVec3* pObject);
void frPerspective(float matrix[16], float fov, float aspect, float near, float far);

#endif
