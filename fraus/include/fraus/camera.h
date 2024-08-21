#ifndef FRAUS_CAMERA_H
#define FRAUS_CAMERA_H

#include "./math.h"

typedef struct FrVulkanData FrVulkanData;

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

extern FrCamera camera;

/*
 *  Create a default camera with close near plane and infinite far plane.
 */
void frCreateCamera(void);

/*
 * Get the camera view and projection matrix.
 *
 * Parameters:
 * - matrix: The matrix to store the result.
 */
void frGetCameraMatrix(float matrix[16]);

#endif
