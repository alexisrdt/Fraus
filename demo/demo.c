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
 * - capture: when true, the mouse is captured
 */
static bool sameForward = false;
static bool capture = true;

// Light position
static FrVec3 lightPosition;

/*
 * Handle key events.
 *
 * Parameters:
 * - key: The key.
 * - state: The key state.
 * - pUserData: Not used.
 */
void myKeyHandler(FrKey key, FrKeyState state, void* pUserData)
{
	(void)pUserData;

	if(state == FR_KEY_STATE_UP)
	{
		return;
	}

	switch(key)
	{
		case FR_KEY_ESCAPE:
			frCloseWindow();
			break;

		case FR_KEY_F:
			frMaximizeWindow();
			break;

		case FR_KEY_P:
			sameForward = !sameForward;
			break;

		case FR_KEY_M:
			capture = !capture;
			frCaptureMouse(capture);
			break;

		case FR_KEY_LEFT_MOUSE:
			if(!capture)
			{
				capture = true;
				frCaptureMouse(capture);
			}
			break;

		default:
			break;
	}
}

/*
 * Mouse move handler called each time the mouse moves.
 *
 * Parameters:
 * - dx: Mouse movement on the X axis.
 * - dy: Mouse movement on the Y axis.
 * - pUserData: Not used.
 */
void myMouseMoveHandler(int32_t dx, int32_t dy, void* pUserData)
{
	(void)pUserData;

	// Update yaw
	camera.yaw = fmodf(camera.yaw - dx * camera.rotationSpeed * capture, 2.f * PI);

	// Update pitch
	camera.pitch += dy * camera.rotationSpeed * capture;
	camera.pitch = camera.pitch < 0.001f ? 0.001f : camera.pitch;
	camera.pitch = camera.pitch > PI - 0.001f ? PI - 0.001f : camera.pitch;
}

// Define number of objects and buffer for lowest Zs
#define OBJECT_COUNT 5
static float lowestZs[OBJECT_COUNT];

/*
 * Update handler called every frame.
 *
 * Parameters:
 * - elapsed: Elapsed time since last frame in seconds.
 */
void myUpdateHandler(float elapsed, void* pUserData)
{
	(void)pUserData;

	if(frGamepad.available)
	{
		camera.yaw = fmodf(camera.yaw - frGamepad.rightStickX * elapsed * camera.rotationSpeed * 1000, 2.f * PI);

		camera.pitch -= frGamepad.rightStickY * elapsed * camera.rotationSpeed * 1000;
		camera.pitch = camera.pitch < 0.001f ? 0.001f : camera.pitch;
		camera.pitch = camera.pitch > PI - 0.001f ? PI - 0.001f : camera.pitch;
	}

	static float time = 0.f;
	time = fmodf(time + elapsed, 2.f * PI);

	float angle = 2.f * time;

	lightPosition.x = -3.f + 1.5f * cosf(angle);
	lightPosition.y = -3.f + 1.5f * sinf(angle);

	// World up vector
	const FrVec3 worldUp = {.x = 0.f, .y = 0.f, .z = 1.f};

	// Set model matrix to identity
	frIdentity(frObjects.data[0].transformation);
	frTranslation(frObjects.data[1].transformation, 0.f, 0.f, -lowestZs[1]);
	frTranslation(frObjects.data[2].transformation, -3.f, -3.f, -lowestZs[2]);
	frTranslation(frObjects.data[3].transformation, 4.f, -5.f, -lowestZs[3]);
	float rotation[16];
	frZRotation(rotation, angle);
	float translation[16];
	frTranslation(translation, lightPosition.x, lightPosition.y, lightPosition.z);
	frMultiply(rotation, translation, frObjects.data[4].transformation);

	float scale[16];
	frScaling(scale, 0.5f, 1.f, 1.0f);
	frTranslation(translation, 0.f, 0.f, 5.f);
	frMultiply(scale, translation, frObjects.data[5].transformation);

	// Compute camera's objective
	const FrVec3 objective = {
		.x = camera.position.x + cosf(camera.yaw) * sinf(camera.pitch),
		.y = camera.position.y + sinf(camera.yaw) * sinf(camera.pitch),
		.z = camera.position.z + cosf(camera.pitch)
	};

	// Set camera speed
	camera.translationSpeed = 3.f;
	if(frGetKeyState(FR_KEY_LEFT_MOUSE) == FR_KEY_STATE_DOWN || (frGamepad.available && frGamepad.leftTrigger > 0.5f))
	{
		camera.translationSpeed = 0.5f;
	}
	else if(frGetKeyState(FR_KEY_RIGHT_MOUSE) == FR_KEY_STATE_DOWN || (frGamepad.available && frGamepad.rightTrigger > 0.5f))
	{
		camera.translationSpeed = 10.f;
	}

	// Compute movement vectors
	FrVec3 movForward = frSubstract(&objective, &camera.position);
	if(!sameForward)
	{
		movForward.z = 0.f;
	}
	frNormalize(&movForward);

	FrVec3 movRight = frCross(&movForward, &worldUp);
	frNormalize(&movRight);

	const FrVec3 movUp = frCross(&movRight, &movForward);
	// No need to normalize, forward and right are orthonormal

	// Move camera
	if(frGetKeyState(FR_KEY_Z) == FR_KEY_STATE_DOWN || frGetKeyState(FR_KEY_UP) == FR_KEY_STATE_DOWN)
	{
		const FrVec3 movement = frScale(&movForward, elapsed * camera.translationSpeed);
		camera.position = frAdd(&camera.position, &movement);
	}

	if(frGetKeyState(FR_KEY_S) == FR_KEY_STATE_DOWN || frGetKeyState(FR_KEY_DOWN) == FR_KEY_STATE_DOWN)
	{
		const FrVec3 movement = frScale(&movForward, -elapsed * camera.translationSpeed);
		camera.position = frAdd(&camera.position, &movement);
	}

	if(frGetKeyState(FR_KEY_Q) == FR_KEY_STATE_DOWN || frGetKeyState(FR_KEY_LEFT) == FR_KEY_STATE_DOWN)
	{
		const FrVec3 movement = frScale(&movRight, -elapsed * camera.translationSpeed);
		camera.position = frAdd(&camera.position, &movement);
	}

	if(frGetKeyState(FR_KEY_D) == FR_KEY_STATE_DOWN || frGetKeyState(FR_KEY_RIGHT) == FR_KEY_STATE_DOWN)
	{
		const FrVec3 movement = frScale(&movRight, elapsed * camera.translationSpeed);
		camera.position = frAdd(&camera.position, &movement);
	}

	if(frGetKeyState(FR_KEY_SPACE) == FR_KEY_STATE_DOWN)
	{
		const FrVec3 movement = frScale(&movUp, elapsed * camera.translationSpeed);
		camera.position = frAdd(&camera.position, &movement);
	}

	if(frGetKeyState(FR_KEY_LEFT_CONTROL) == FR_KEY_STATE_DOWN)
	{
		const FrVec3 movement = frScale(&movUp, -elapsed * camera.translationSpeed);
		camera.position = frAdd(&camera.position, &movement);
	}

	if(frGamepad.available)
	{
		const FrVec3 movementForward = frScale(&movForward, frGamepad.leftStickY * elapsed * camera.translationSpeed);
		const FrVec3 movementRight = frScale(&movRight, frGamepad.leftStickX * elapsed * camera.translationSpeed);
		const FrVec3 movementUp = frScale(&movUp, elapsed * camera.translationSpeed * frGamepad.buttonDown);
		const FrVec3 movementDown = frScale(&movUp, -elapsed * camera.translationSpeed * frGamepad.buttonRight);
		FrVec3 movement = frAdd(&movementForward, &movementRight);
		movement = frAdd(&movement, &movementUp);
		movement = frAdd(&movement, &movementDown);

		camera.position = frAdd(&camera.position, &movement);
	}

	memcpy(uniformBuffers.data[1].bufferDatas[frameInFlightIndex], &lightPosition, sizeof(lightPosition));
}

#define TRUC 10.f
float widthToVk(float size)
{
	return size / swapchainExtent.width * 2.f / TRUC;
}
float heightToVk(float size)
{
	return size / swapchainExtent.height * 2.f / TRUC;
}

/*
 * Main function.
 * Create and run the application.
 */
int main(void)
{
	lightPosition.z = 4.f;

	// Create application
	if(frCreateApplication("My super Fraus application", 1)!= FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	// Add input handlers
	frSetKeyHandler(myKeyHandler, NULL);
	frSetMouseMoveHandler(myMouseMoveHandler, NULL);
	frSetUpdateHandler(myUpdateHandler, NULL);

	// Capture mouse
	frCaptureMouse(capture);

	// Initialize camera
	camera.position = (FrVec3){.x = 2.f, .y = 2.f, .z = 2.f};
	camera.yaw = 225.f * PI / 180.f;
	camera.pitch = 2.f * PI / 3.f;

	// Create pipelines
	FrPipelineCreateInfo pipelineInfo = {
		.vertexShaderPath = "shader_vert.spv",
		.fragmentShaderPath = "shader_frag.spv"
	};
	if(frCreateGraphicsPipeline(&pipelineInfo) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	pipelineInfo.vertexShaderPath = "phong_vert.spv";
	pipelineInfo.fragmentShaderPath = "phong_frag.spv";
	if(frCreateGraphicsPipeline(&pipelineInfo) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	// Create lightPosition uniform buffer
	if(frCreateUniformBuffer(sizeof(lightPosition)) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	// Create objects
	#define BUFFER_SIZE 32
	char modelFileName[BUFFER_SIZE];
	char textureFileName[BUFFER_SIZE];
	for(uint32_t objectIndex = 0; objectIndex < OBJECT_COUNT; ++objectIndex)
	{
		if(snprintf(modelFileName, BUFFER_SIZE, "assets/model_%d.obj", objectIndex) < 0)
		{
			return EXIT_FAILURE;
		}
		if(snprintf(textureFileName, BUFFER_SIZE, "assets/texture_%d.png", objectIndex) < 0)
		{
			return EXIT_FAILURE;
		}

		if(frCreateTexture(textureFileName) != FR_SUCCESS) return EXIT_FAILURE;
		if(frCreateObject(
			modelFileName,
			objectIndex == 2 ? 1 : 0,
			objectIndex == 2 ? (uint32_t[]){0, objectIndex, 1} : (uint32_t[]){0, objectIndex}
		) != FR_SUCCESS)
		{
			return EXIT_FAILURE;
		}

		lowestZs[objectIndex] = FLT_MAX;
		for(uint32_t vertexIndex = 0; vertexIndex < frObjects.data[objectIndex].vertexCount; ++vertexIndex)
		{
			if(frObjects.data[objectIndex].vertices[vertexIndex].position.z < lowestZs[objectIndex])
			{
				lowestZs[objectIndex] = frObjects.data[objectIndex].vertices[vertexIndex].position.z;
			}
		}
	}

	// Text test
	pipelineInfo.vertexShaderPath = "text_vert.spv";
	pipelineInfo.fragmentShaderPath = "text_frag.spv";
	const VkVertexInputRate vertexInputRates[] = {VK_VERTEX_INPUT_RATE_VERTEX, VK_VERTEX_INPUT_RATE_INSTANCE};
	const uint32_t vertexInputStrides[] = {8, 20};
	pipelineInfo.vertexInputRateCount = FR_LEN(vertexInputRates);
	pipelineInfo.vertexInputRates = vertexInputRates;
	pipelineInfo.vertexInputStrides = vertexInputStrides;
	pipelineInfo.depthTestDisable = true;
	pipelineInfo.alphaBlendEnable = true;
	if(frCreateGraphicsPipeline(&pipelineInfo) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	FrFont font;
	if(frLoadFont("assets/font.ttf", &font) != FR_SUCCESS)
	{
		printf("Failed to load font\n");
		return EXIT_FAILURE;
	}

	const size_t contourInfoSize = font.contourInfoCount * sizeof(font.contourInfos[0]);
	if(frCreateStorageBuffer(contourInfoSize) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frSetStorageBufferData(0, font.contourInfos, contourInfoSize) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	const size_t offsetsSize = font.glyphCount * sizeof(font.glyphOffsets[0]) * 2;
	if(frCreateStorageBuffer(offsetsSize) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frSetStorageBufferData(1, font.glyphOffsets, offsetsSize) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	const size_t pointsSize = font.pointCount * sizeof(font.points[0]) * 2;
	if(frCreateStorageBuffer(pointsSize) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frSetStorageBufferData(2, font.points, pointsSize) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	if(frCreateObject("assets/model_0.obj", 2, (uint32_t[]){1, 0, 2}) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	const char text[] = "Hélloij, ç*¤£$µù%!";
	const size_t textLength = FR_LEN(text) - 1;
	const FrVec2 textQuadPoints[] = {
		{0.f, 0.f},
		{1.f, 0.f},
		{1.f, 1.f},
		{0.f, 1.f}
	};
	const uint16_t textQuadIndices[] = {
		0, 1, 2,
		2, 3, 0
	};

	uint32_t glyphCount = 0;
	uint32_t glyphIds[FR_LEN(text) - 1];

	FrStringReader reader = {
		.string = text,
		.end = text + textLength
	};
	while(reader.string < reader.end)
	{
		uint32_t characterCode;
		if(frNextCharacterCode(&reader, &characterCode) != FR_SUCCESS)
		{
			return EXIT_FAILURE;
		}

		if(frGetGlyphId(&font, characterCode, &glyphIds[glyphCount]) != FR_SUCCESS)
		{
			return EXIT_FAILURE;
		}

		++glyphCount;
	}

	float* const truc = malloc(glyphCount * 4 * sizeof(truc[0]));
	if(!truc)
	{
		return EXIT_FAILURE;
	}
	uint32_t* const truc2 = malloc(glyphCount * sizeof(truc2[0]));
	if(!truc2)
	{
		free(truc);
		return EXIT_FAILURE;
	}
	float xOffset = -1.f;
	uint32_t j = 0;
	for(uint32_t i = 0; i < glyphCount; ++i)
	{
		const int16_t width = font.glyphPositions[glyphIds[i]].xMax - font.glyphPositions[glyphIds[i]].xMin;
		const int16_t height = font.glyphPositions[glyphIds[i]].yMax - font.glyphPositions[glyphIds[i]].yMin;
		if(width != 0 && height != 0)
		{
			truc[j * 4] = xOffset + widthToVk(font.glyphPositions[glyphIds[i]].leftSideBearing);
			truc[j * 4 + 1] = -heightToVk(font.glyphPositions[glyphIds[i]].yMin) - heightToVk(height);
			truc[j * 4 + 2] = widthToVk(width);
			truc[j * 4 + 3] = heightToVk(height);
			truc2[j] = glyphIds[i];
			++j;
		}
		xOffset += widthToVk(font.glyphPositions[glyphIds[i]].advanceWidth);
	};
	instanceCount = j;
	const VkDeviceSize s = sizeof(textQuadPoints) + sizeof(textQuadIndices) + (instanceCount * 4 * sizeof(truc[0])) + (instanceCount * truc2[0]);
	if(frCreateBuffer(s, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &instanceBuffer, &instanceBufferMemory) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	void* data;
	if(vkMapMemory(device, instanceBufferMemory, 0, s, 0, &data) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	char* trucData = (char*)data;
	memcpy(trucData, textQuadPoints, sizeof(textQuadPoints));
	trucData += sizeof(textQuadPoints);
	memcpy(trucData, textQuadIndices, sizeof(textQuadIndices));
	trucData += sizeof(textQuadIndices);
	for(uint32_t i = 0; i < instanceCount; ++i)
	{
		memcpy(trucData, truc + 4 * i, sizeof(truc[0]) * 4);
		trucData += sizeof(truc[0]) * 4;
		memcpy(trucData, truc2 + i, sizeof(truc2[0]));
		trucData += sizeof(truc2[0]);
	}
	vkUnmapMemory(device, instanceBufferMemory);
	free(truc);
	free(truc2);

	if(frFreeFont(&font) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	// Main loop
	const int exitValue = frRunApplication();

	printf("Exiting with exit value %d\n", exitValue);
	return exitValue;
}
