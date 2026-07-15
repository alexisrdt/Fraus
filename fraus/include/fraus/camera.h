#ifndef FRAUS_CAMERA_H
#define FRAUS_CAMERA_H

#include "./math.h"
#include "./vulkan/include.h"

typedef struct FrCamera
{
	FrVec3 position;

	float yaw;
	float pitch;

	float nearPlane;
	float farPlane;

	float translationSpeed;
	float rotationSpeed;
} FrCamera;

/*
 *  Create a default camera with close near plane and infinite far plane.
 */
void frCreateCamera(FrCamera* camera);

/*
 * Get the camera view and projection matrix.
 *
 * Parameters:
 * - matrix: The matrix to store the result.
 */
void frGetCameraMatrix(const FrCamera* camera, const FrEngine* engine, float matrix[16]);

#endif
