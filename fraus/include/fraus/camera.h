#ifndef FRAUS_CAMERA_H
#define FRAUS_CAMERA_H

#include "./math.h"

typedef struct FrVulkanData FrVulkanData;

typedef struct FrCamera
{
	FrVulkanData* pVulkanData;

	FrVec3 position;

	float yaw;
	float pitch;

	float nearPlane;
	float farPlane;

	float translationSpeed;
	float rotationSpeed;
} FrCamera;

/*
 *  Creates a default camera with close near plane and infinite far plane
 *  - pCamera: pointer to a camera
 */
void frCreateCamera(FrCamera* pCamera, FrVulkanData* pVulkanData);

/*
 * Get the camera view and projection matrix
 */
void frGetCameraMatrix(FrCamera* pCamera, float matrix[16]);

#endif
