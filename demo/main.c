#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fraus/fraus.h>

/*
 * Define flags
 * - sameForward: when true, the camera moves in the direction where it is looking
 * - maximized: when true, the window is maximized
 * - capture: when true, the mouse is captured
 */
static bool sameForward = false;
static bool maximized = false;
static bool capture = true;

// Color factor
static FrVec3 factor = {0};

/*
 * Key handler called each time a key is pressed or released
 * - key: the key
 * - state: the key state
 * - pWindow: pointer to the window
 */
void myKeyHandler(FrKey key, FrKeyState state, FrWindow* pWindow)
{
	if(state == FR_KEY_STATE_UP) return;

	switch(key)
	{
		case FR_KEY_ESCAPE:
			frDestroyWindow(pWindow);
			break;

		case FR_KEY_P:
			sameForward = !sameForward;
			break;

		case FR_KEY_F:
			maximized = !maximized;
			ShowWindow(pWindow->handle, maximized ? SW_MAXIMIZE : SW_NORMAL);
			break;

		case FR_KEY_M:
			capture = !capture;
			frCaptureMouse(pWindow, capture);
			break;

		case FR_KEY_LEFT_MOUSE:
			if(!capture)
			{
				capture = true;
				frCaptureMouse(pWindow, capture);
			}
			break;
	}
}

/*
 * Mouse move handler called each time the mouse moves
 * - dx: mouse movement on the X axis
 * - dy: mouse movement on the Y axis
 * - pCamera: pointer to the camera
 */
void myMouseMoveHandler(int32_t dx, int32_t dy, FrCamera* pCamera)
{
	if(capture)
	{
		// Update yaw
		pCamera->yaw -= (float)dx * pCamera->rotationSpeed;
		pCamera->yaw = fmodf(pCamera->yaw, 2.f * PI);

		// Update pitch
		pCamera->pitch += (float)dy * pCamera->rotationSpeed;
		if(pCamera->pitch < 0.01f) pCamera->pitch = 0.01f;
		if(pCamera->pitch > PI - 0.01f) pCamera->pitch = PI - 0.01f;
	}
}

// Define number of objects and buffer for lowest Zs
#define OBJECT_COUNT 4
static float lowestZs[OBJECT_COUNT];

/*
 * Update handler called every frame
 * - pVulkanData: pointer to the Vulkan data
 * - elapsed: elapsed time since last frame in seconds
 * - pCamera: pointer to the camera
 */
void myUpdateHandler(FrVulkanData* pVulkanData, float elapsed, FrCamera* pCamera)
{
	// World up vector
	static const FrVec3 worldUp = {.x = 0.f, .y = 0.f, .z = 1.f};

	// Set model matrix to identity
	frIdentity(pVulkanData->objects.pData[0].transformation);
	frTranslation(pVulkanData->objects.pData[1].transformation, 0.f, 0.f, -lowestZs[1]);
	frTranslation(pVulkanData->objects.pData[2].transformation, -3.f, -3.f, -lowestZs[2]);
	frTranslation(pVulkanData->objects.pData[3].transformation, 4.f, -5.f, -lowestZs[3]);

	// Compute camera's objective
	const FrVec3 objective = {
		.x = pCamera->position.x + cosf(pCamera->yaw) * sinf(pCamera->pitch),
		.y = pCamera->position.y + sinf(pCamera->yaw) * sinf(pCamera->pitch),
		.z = pCamera->position.z + cosf(pCamera->pitch)
	};

	// Set camera speed
	pCamera->translationSpeed = 3.f;
	if(frGetKeyState(FR_KEY_LEFT_MOUSE) == FR_KEY_STATE_DOWN) pCamera->translationSpeed = 0.5f;
	else if(frGetKeyState(FR_KEY_RIGHT_MOUSE) == FR_KEY_STATE_DOWN) pCamera->translationSpeed = 10.f;

	// Compute movement vectors
	FrVec3 movForward = frSubstract(&objective, &pCamera->position);
	if(!sameForward) movForward.z = 0.f;
	frNormalize(&movForward);

	FrVec3 movRight = frCross(&movForward, &worldUp);
	frNormalize(&movRight);

	const FrVec3 movUp = frCross(&movRight, &movForward);
	// No need to normalize, forward and right are orthonormal

	// Move camera
	if(
		frGetKeyState(FR_KEY_Z) == FR_KEY_STATE_DOWN ||
		frGetKeyState(FR_KEY_UP) == FR_KEY_STATE_DOWN
	)
	{
		const FrVec3 movement = frScale(&movForward, elapsed * pCamera->translationSpeed);
		pCamera->position = frAdd(&pCamera->position, &movement);
	}

	if(
		frGetKeyState(FR_KEY_S) == FR_KEY_STATE_DOWN ||
		frGetKeyState(FR_KEY_DOWN) == FR_KEY_STATE_DOWN
	)
	{
		const FrVec3 movement = frScale(&movForward, -elapsed * pCamera->translationSpeed);
		pCamera->position = frAdd(&pCamera->position, &movement);
	}

	if(
		frGetKeyState(FR_KEY_Q) == FR_KEY_STATE_DOWN ||
		frGetKeyState(FR_KEY_LEFT) == FR_KEY_STATE_DOWN
	)
	{
		const FrVec3 movement = frScale(&movRight, -elapsed * pCamera->translationSpeed);
		pCamera->position = frAdd(&pCamera->position, &movement);
	}

	if(
		frGetKeyState(FR_KEY_D) == FR_KEY_STATE_DOWN ||
		frGetKeyState(FR_KEY_RIGHT) == FR_KEY_STATE_DOWN
	)
	{
		const FrVec3 movement = frScale(&movRight, elapsed * pCamera->translationSpeed);
		pCamera->position = frAdd(&pCamera->position, &movement);
	}

	if(
		frGetKeyState(FR_KEY_SPACE) == FR_KEY_STATE_DOWN
	)
	{
		const FrVec3 movement = frScale(&movUp, elapsed * pCamera->translationSpeed);
		pCamera->position = frAdd(&pCamera->position, &movement);
	}

	if(
		frGetKeyState(FR_KEY_LEFT_CONTROL) == FR_KEY_STATE_DOWN
	)
	{
		const FrVec3 movement = frScale(&movUp, -elapsed * pCamera->translationSpeed);
		pCamera->position = frAdd(&pCamera->position, &movement);
	}

	// Update color factor
	if(
		frGetKeyState(FR_KEY_R) == FR_KEY_STATE_DOWN
	)
	{
		factor.r += elapsed;
		if(factor.r > 1.f) factor.r = 1.f;
	}

	if(
		frGetKeyState(FR_KEY_T) == FR_KEY_STATE_DOWN
	)
	{
		factor.r -= elapsed;
		if(factor.r < 0.f) factor.r = 0.f;
	}

	if(
		frGetKeyState(FR_KEY_G) == FR_KEY_STATE_DOWN
	)
	{
		factor.g += elapsed;
		if(factor.g > 1.f) factor.g = 1.f;
	}

	if(
		frGetKeyState(FR_KEY_H) == FR_KEY_STATE_DOWN
	)
	{
		factor.g -= elapsed;
		if(factor.g < 0.f) factor.g = 0.f;
	}

	if(
		frGetKeyState(FR_KEY_B) == FR_KEY_STATE_DOWN
	)
	{
		factor.b += elapsed;
		if(factor.b > 1.f) factor.b = 1.f;
	}

	if(
		frGetKeyState(FR_KEY_N) == FR_KEY_STATE_DOWN
	)
	{
		factor.b -= elapsed;
		if(factor.b < 0.f) factor.b = 0.f;
	}

	memcpy(pVulkanData->uniformBuffers.pData[1].pBufferDatas[pVulkanData->frameInFlightIndex], &factor, sizeof(factor));
}

/*
 * Main function. Creates and runs the application, then cleans up.
 */
int main(void)
{
	// Create application
	FrApplication application;
	if(frCreateApplication("My super Fraus application", 1, &application) != FR_SUCCESS) return EXIT_FAILURE;

	// Add input handlers
	frSetKeyHandler(&application.window, myKeyHandler, &application.window);
	frSetMouseMoveHandler(&application.window, myMouseMoveHandler, &application.camera);
	frSetUpdateHandler(&application.vulkanData, myUpdateHandler, &application.camera);

	// Capture mouse
	frCaptureMouse(&application.window, capture);

	// Initialize camera
	application.camera.position = (FrVec3){ .x = 2.f, .y = 2.f, .z = 2.f };
	application.camera.yaw = 225.f * PI / 180.f;
	application.camera.pitch = 2.f * PI / 3.f;

	// Create pipelines
	VkDescriptorSetLayoutBinding bindings[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
		},
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
		}
	};
	if(frCreateGraphicsPipeline(&application.vulkanData, "vert.spv", "frag.spv", bindings, sizeof(bindings) / sizeof(bindings[0])) != FR_SUCCESS) return EXIT_FAILURE;

	VkDescriptorSetLayoutBinding phongBindings[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
		},
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
		},
		{
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
		}
	};
	if(frCreateGraphicsPipeline(&application.vulkanData, "vert.spv", "other.spv", phongBindings, sizeof(phongBindings) / sizeof(phongBindings[0])) != FR_SUCCESS) return EXIT_FAILURE;

	// Create color factor uniform buffer
	if(frCreateUniformBuffer(&application.vulkanData, sizeof(FrVec3)) != FR_SUCCESS) return EXIT_FAILURE;

	// Create objects
#define BUFFER_SIZE 32
	char modelFileName[BUFFER_SIZE];
	char textureFileName[BUFFER_SIZE];
	for(uint32_t objectIndex = 0; objectIndex < OBJECT_COUNT; ++objectIndex)
	{
		if(snprintf(modelFileName, BUFFER_SIZE, "model_%d.obj", objectIndex) < 0) return EXIT_FAILURE;
		if(snprintf(textureFileName, BUFFER_SIZE, "texture_%d.png", objectIndex) < 0) return EXIT_FAILURE;

		if(frCreateTexture(&application.vulkanData, textureFileName) != FR_SUCCESS) return EXIT_FAILURE;
		if(frCreateObject(
			&application.vulkanData,
			modelFileName,
			objectIndex == 2 ? 1 : 0,
			objectIndex == 2 ? (uint32_t[]) { 0, objectIndex, 1 } : (uint32_t[]) { 0, objectIndex }
		) != FR_SUCCESS) return EXIT_FAILURE;

		lowestZs[objectIndex] = FLT_MAX;
		for(uint32_t vertexIndex = 0; vertexIndex < application.vulkanData.objects.pData[objectIndex].vertexCount; ++vertexIndex)
		{
			if(application.vulkanData.objects.pData[objectIndex].pVertices[vertexIndex].position.z < lowestZs[objectIndex])
			{
				lowestZs[objectIndex] = application.vulkanData.objects.pData[objectIndex].pVertices[vertexIndex].position.z;
			}
		}
	}

	// Main loop
	const int exitValue = frMainLoop(&application);

	// Clean up
	if(frDestroyApplication(&application) != FR_SUCCESS) return EXIT_FAILURE;

	return exitValue;
}
