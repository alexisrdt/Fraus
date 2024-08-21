#include "../include/fraus/camera.h"

#include <math.h>

#include "../include/fraus/fraus.h"

FrCamera camera;

void frCreateCamera(void)
{
	camera.position.x = 0.f;
	camera.position.y = 0.f;
	camera.position.z = 0.f;

	camera.yaw = 0.f;
	camera.pitch = 0.f;

	camera.nearPlane = 0.01f;
	camera.farPlane = -1.f;

	camera.translationSpeed = 3.f;
	camera.rotationSpeed = 0.003f;
}

void frGetCameraMatrix(float matrix[16])
{
	// Compute objective
	const FrVec3 objective = {
		.x = camera.position.x + cosf(camera.yaw) * sinf(camera.pitch),
		.y = camera.position.y + sinf(camera.yaw) * sinf(camera.pitch),
		.z = camera.position.z + cosf(camera.pitch)
	};

	// Compute view matrix
	float viewMatrix[16];
	frLookAt(viewMatrix, &camera.position, &objective);

	float perspectiveMatrix[16];
	if(camera.farPlane < 0.f)
	{
		frPerspectiveInfiniteFar(
			perspectiveMatrix,
			PI / 4.f,
			(float)swapchainExtent.width / (float)swapchainExtent.height,
			camera.nearPlane
		);
	}
	else
	{
		frPerspective(
			perspectiveMatrix,
			PI / 4.f,
			(float)swapchainExtent.width / (float)swapchainExtent.height,
			camera.nearPlane,
			camera.farPlane
		);
	}

	frMultiply(viewMatrix, perspectiveMatrix, matrix);
}
