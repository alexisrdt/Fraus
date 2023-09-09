#include "camera.h"

#include <math.h>

#include "fraus.h"

void frCreateCamera(FrCamera* pCamera, FrVulkanData* pVulkanData)
{
	pCamera->pVulkanData = pVulkanData;

	pCamera->position = (FrVec3){0.f, 0.f, 0.f};

	pCamera->yaw = 0.f;
	pCamera->pitch = 0.f;

	pCamera->nearPlane = 0.01f;
	pCamera->farPlane = -1.f;

	pCamera->translationSpeed = 3.f;
	pCamera->rotationSpeed = 0.003f;
}

void frGetCameraMatrix(FrCamera* pCamera, float matrix[16])
{
	// Set world up direction
	static const FrVec3 worldUp = {.x = 0.f, .y = 0.f, .z = 1.f};

	// Compute objective
	const FrVec3 objective = {
		.x = pCamera->position.x + cosf(pCamera->yaw) * sinf(pCamera->pitch),
		.y = pCamera->position.y + sinf(pCamera->yaw) * sinf(pCamera->pitch),
		.z = pCamera->position.z + cosf(pCamera->pitch)
	};

	// Compute direction vectors
	FrVec3 forward = frSubstract(&objective, &pCamera->position);
	frNormalize(&forward);

	FrVec3 right = frCross(&forward, &worldUp);
	frNormalize(&right);

	const FrVec3 up = frCross(&right, &forward);
	// No need to normalize, forward and right are orthonormal

	// Compute matrix
	static float viewMatrix[16];
	frLookDir(viewMatrix, &pCamera->position, &forward, &right, &up);

	static float perspectiveMatrix[16];
	if(pCamera->farPlane < 0.f)
	{
		frPerspectiveInfiniteFar(
			perspectiveMatrix,
			PI / 4.f,
			(float)pCamera->pVulkanData->extent.width / (float)pCamera->pVulkanData->extent.height,
			pCamera->nearPlane
		);
	}
	else
	{
		frPerspective(
			perspectiveMatrix,
			PI / 4.f,
			(float)pCamera->pVulkanData->extent.width / (float)pCamera->pVulkanData->extent.height,
			pCamera->nearPlane,
			pCamera->farPlane
		);
	}

	frMultiply(viewMatrix, perspectiveMatrix, matrix);
}
