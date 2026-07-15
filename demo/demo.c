#include <float.h>
#include <math.h>
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
 * - userData: Not used.
 */
void myKeyHandler(const FrKey key, const FrKeyState state, void* const windowVoid)
{
	FrWindow* const window = windowVoid;

	if(state == FR_KEY_STATE_UP)
	{
		return;
	}

	switch(key)
	{
		case FR_KEY_ESCAPE:
			frCloseWindow(window);
			break;

		case FR_KEY_F:
			frMaximizeWindow(window);
			break;

		case FR_KEY_P:
			sameForward = !sameForward;
			break;

		case FR_KEY_M:
			capture = !capture;
			frCaptureMouse(window, capture);
			break;

		case FR_KEY_LEFT_MOUSE:
			if(!capture)
			{
				capture = true;
				frCaptureMouse(window, capture);
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
 * - userData: Not used.
 */
void myMouseMoveHandler(const int32_t dx, const int32_t dy, void* const cameraVoid)
{
	FrCamera* const camera = cameraVoid;

	// Update yaw
	camera->yaw = fmodf(camera->yaw - dx * camera->rotationSpeed * capture, 2.f * PI);

	// Update pitch
	camera->pitch += dy * camera->rotationSpeed * capture;
	camera->pitch = camera->pitch < 0.001f ? 0.001f : camera->pitch;
	camera->pitch = camera->pitch > PI - 0.001f ? PI - 0.001f : camera->pitch;
}

// Define number of objects and buffer for lowest Zs
#define OBJECT_COUNT 5
static float lowestZs[OBJECT_COUNT];

/*
 * Update handler called every frame.
 *
 * Parameters:
 * - elapsed: Elapsed time since last frame in seconds.
 * - userData: Not used.
 */
void myUpdateHandler(const float elapsed, void* const applicationVoid)
{
	FrApplication* const application = applicationVoid;

	static float time = 0.f;
	time = fmodf(time + elapsed, 2.f * PI);

	float angle = 2.f * time;

	lightPosition.x = -3.f + 1.5f * cosf(angle);
	lightPosition.y = -3.f + 1.5f * sinf(angle);

	// World up vector
	const FrVec3 worldUp = {.x = 0.f, .y = 0.f, .z = 1.f};

	// Set model matrix to identity
	float scaling[16];
	frScaling(scaling, 10.f, 10.f, 1.f);
	float translation[16];
	frTranslation(translation, -5.f, -5.f, 0.f);
	frMultiply(scaling, translation, application->engine.objects[0].transformation);
	frTranslation(application->engine.objects[1].transformation, 0.f, 0.f, -lowestZs[1]);
	frTranslation(application->engine.objects[2].transformation, -3.f, -3.f, -lowestZs[2]);
	frTranslation(application->engine.objects[3].transformation, 4.f, -5.f, -lowestZs[3]);
	float rotation[16];
	frZRotation(rotation, angle);
	frTranslation(translation, lightPosition.x, lightPosition.y, lightPosition.z);
	frMultiply(rotation, translation, application->engine.objects[4].transformation);

	float scale[16];
	frScaling(scale, 0.5f, 1.f, 1.0f);
	frTranslation(translation, 0.f, 0.f, 5.f);
	frMultiply(scale, translation, application->engine.objects[5].transformation);

	// Compute camera's objective
	const FrVec3 objective = {
		.x = application->camera.position.x + cosf(application->camera.yaw) * sinf(application->camera.pitch),
		.y = application->camera.position.y + sinf(application->camera.yaw) * sinf(application->camera.pitch),
		.z = application->camera.position.z + cosf(application->camera.pitch)
	};

	// Set camera speed
	application->camera.translationSpeed = 3.f;
	if(frGetKeyState(FR_KEY_LEFT_MOUSE) == FR_KEY_STATE_DOWN)
	{
		application->camera.translationSpeed = 0.5f;
	}
	else if(frGetKeyState(FR_KEY_RIGHT_MOUSE) == FR_KEY_STATE_DOWN)
	{
		application->camera.translationSpeed = 10.f;
	}

	// Compute movement vectors
	FrVec3 movForward = frSubstract(&objective, &application->camera.position);
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
		const FrVec3 movement = frScale(&movForward, elapsed * application->camera.translationSpeed);
		application->camera.position = frAdd(&application->camera.position, &movement);
	}

	if(frGetKeyState(FR_KEY_S) == FR_KEY_STATE_DOWN || frGetKeyState(FR_KEY_DOWN) == FR_KEY_STATE_DOWN)
	{
		const FrVec3 movement = frScale(&movForward, -elapsed * application->camera.translationSpeed);
		application->camera.position = frAdd(&application->camera.position, &movement);
	}

	if(frGetKeyState(FR_KEY_Q) == FR_KEY_STATE_DOWN || frGetKeyState(FR_KEY_LEFT) == FR_KEY_STATE_DOWN)
	{
		const FrVec3 movement = frScale(&movRight, -elapsed * application->camera.translationSpeed);
		application->camera.position = frAdd(&application->camera.position, &movement);
	}

	if(frGetKeyState(FR_KEY_D) == FR_KEY_STATE_DOWN || frGetKeyState(FR_KEY_RIGHT) == FR_KEY_STATE_DOWN)
	{
		const FrVec3 movement = frScale(&movRight, elapsed * application->camera.translationSpeed);
		application->camera.position = frAdd(&application->camera.position, &movement);
	}

	if(frGetKeyState(FR_KEY_SPACE) == FR_KEY_STATE_DOWN)
	{
		const FrVec3 movement = frScale(&movUp, elapsed * application->camera.translationSpeed);
		application->camera.position = frAdd(&application->camera.position, &movement);
	}

	if(frGetKeyState(FR_KEY_LEFT_CONTROL) == FR_KEY_STATE_DOWN)
	{
		const FrVec3 movement = frScale(&movUp, -elapsed * application->camera.translationSpeed);
		application->camera.position = frAdd(&application->camera.position, &movement);
	}

	memcpy(application->engine.uniformBuffers[application->engine.uniformBufferCount - 2].bufferDatas[application->engine.frameInFlightIndex], &lightPosition, sizeof(lightPosition));

	const float extent[2] = {application->engine.swapchainExtent.width, application->engine.swapchainExtent.height};
	memcpy(application->engine.uniformBuffers[application->engine.uniformBufferCount - 1].bufferDatas[application->engine.frameInFlightIndex], extent, sizeof(extent));
}

#define TEXT_SCALING 10.f
float widthToVk(FrEngine* const engine, const float size)
{
	return size / engine->swapchainExtent.width * 2.f / TEXT_SCALING;
}
float heightToVk(FrEngine* const engine, const float size)
{
	return size / engine->swapchainExtent.height * 2.f / TEXT_SCALING;
}

/*
 * Main function.
 * Create and run the application.
 */
int main(void)
{
	if(frInitialize() != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	lightPosition.z = 4.f;

	// Create application.
	FrApplication application;
	if(frCreateApplication(&application, "My super Fraus application", 1) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	// Add input handlers
	frSetKeyHandler(&application, myKeyHandler, &application.window);
	frSetMouseMoveHandler(&application, myMouseMoveHandler, &application.camera);
	frSetUpdateHandler(&application, myUpdateHandler, &application);

	// Capture mouse
	frCaptureMouse(&application.window, capture);

	// Initialize camera
	application.camera.position = (FrVec3){.x = 2.f, .y = 2.f, .z = 2.f};
	application.camera.yaw = 225.f * PI / 180.f;
	application.camera.pitch = 2.f * PI / 3.f;

	if(frReserveGraphicsPipelines(&application.engine, 4) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	if(frReserveUniformBuffers(&application.engine, 3) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	const uint32_t cameraBuffer = 0;

	if(frReserveStorageBuffers(&application.engine, 3) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	if(frReserveTextures(&application.engine, 5) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	if(frReserveObjects(&application.engine, 6) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	// Create pipelines
	const uint32_t shaderPipeline = application.engine.graphicsPipelineCount;
	FrPipelineCreateInfo pipelineInfo = {
		.vertexShaderPath = "shader_vert.spv",
		.fragmentShaderPath = "shader_frag.spv"
	};
	if(frCreateGraphicsPipeline(&application.engine, &pipelineInfo) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	const uint32_t phongPipeline = application.engine.graphicsPipelineCount;
	pipelineInfo.vertexShaderPath = "phong_vert.spv";
	pipelineInfo.fragmentShaderPath = "phong_frag.spv";
	if(frCreateGraphicsPipeline(&application.engine, &pipelineInfo) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	const uint32_t repeatPipeline = application.engine.graphicsPipelineCount;
	pipelineInfo.vertexShaderPath = "repeat_vert.spv";
	pipelineInfo.fragmentShaderPath = "repeat_frag.spv";
	if(frCreateGraphicsPipeline(&application.engine, &pipelineInfo) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	// Create lightPosition uniform buffer
	const uint32_t lightBuffer = application.engine.uniformBufferCount;
	if(frCreateUniformBuffer(&application.engine, sizeof(lightPosition)) != FR_SUCCESS)
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

		if(frCreateTexture(&application.engine, textureFileName) != VK_SUCCESS)
		{
			return EXIT_FAILURE;
		}
		if(frCreateObject(
			&application.engine,
			modelFileName,
			objectIndex == 0 ? repeatPipeline : (objectIndex == 2 ? phongPipeline : shaderPipeline),
			objectIndex == 2 ? (uint32_t[]){cameraBuffer, objectIndex, lightBuffer} : (uint32_t[]){cameraBuffer, objectIndex}
		) != FR_SUCCESS)
		{
			return EXIT_FAILURE;
		}

		lowestZs[objectIndex] = FLT_MAX;
		for(uint32_t vertexIndex = 0; vertexIndex < application.engine.objects[objectIndex].vertexCount; ++vertexIndex)
		{
			if(application.engine.objects[objectIndex].vertices[vertexIndex].position.z < lowestZs[objectIndex])
			{
				lowestZs[objectIndex] = application.engine.objects[objectIndex].vertices[vertexIndex].position.z;
			}
		}
	}

	// Text test
	const uint32_t textPipeline = application.engine.graphicsPipelineCount;
	pipelineInfo.vertexShaderPath = "text_vert.spv";
	pipelineInfo.fragmentShaderPath = "text_frag.spv";
	const VkVertexInputRate vertexInputRates[] = {VK_VERTEX_INPUT_RATE_VERTEX, VK_VERTEX_INPUT_RATE_INSTANCE};
	const uint32_t vertexInputStrides[] = {sizeof(FrVertex), 20};
	pipelineInfo.vertexInputRateCount = FR_LEN(vertexInputRates);
	pipelineInfo.vertexInputRates = vertexInputRates;
	pipelineInfo.vertexInputStrides = vertexInputStrides;
	pipelineInfo.depthTestDisable = true;
	pipelineInfo.alphaBlendEnable = true;
	if(frCreateGraphicsPipeline(&application.engine, &pipelineInfo) != FR_SUCCESS)
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
	if(frCreateStorageBuffer(&application.engine, contourInfoSize) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frSetStorageBufferData(&application.engine, 0, font.contourInfos, contourInfoSize) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	const size_t offsetsSize = font.glyphCount * sizeof(font.glyphOffsets[0]) * 2;
	if(frCreateStorageBuffer(&application.engine, offsetsSize) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frSetStorageBufferData(&application.engine, 1, font.glyphOffsets, offsetsSize) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	const size_t pointsSize = font.pointCount * sizeof(font.points[0]) * 2;
	if(frCreateStorageBuffer(&application.engine, pointsSize) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frSetStorageBufferData(&application.engine, 2, font.points, pointsSize) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	if(frCreateUniformBuffer(&application.engine, 8) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	if(frCreateObject(&application.engine, "assets/model_0.obj", textPipeline, (uint32_t[]){1, 0, 2, 2}) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	FrVulkanObject* const textObject = &application.engine.objects[application.engine.objectCount - 1];

	const char text[] = "Hélloij, ç*¤£$µù%!";
	const size_t textLength = FR_LEN(text) - 1;

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

	const FrArenaSave save = frArenaSave(&application.arena);
	float* const textTransforms = frArenaAllocate(&application.arena, glyphCount * 4 * sizeof(textTransforms[0]), alignof(typeof(textTransforms[0])));
	if(!textTransforms)
	{
		return EXIT_FAILURE;
	}
	uint32_t* const textGlyphIds = frArenaAllocate(&application.arena, glyphCount * sizeof(textGlyphIds[0]), alignof(typeof(textGlyphIds[0])));
	if(!textGlyphIds)
	{
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
			textTransforms[j * 4] = xOffset + widthToVk(&application.engine, font.glyphPositions[glyphIds[i]].leftSideBearing);
			textTransforms[j * 4 + 1] = -heightToVk(&application.engine, font.glyphPositions[glyphIds[i]].yMin) - heightToVk(&application.engine, height);
			textTransforms[j * 4 + 2] = widthToVk(&application.engine, width);
			textTransforms[j * 4 + 3] = heightToVk(&application.engine, height);
			textGlyphIds[j] = glyphIds[i];
			++j;
		}
		xOffset += widthToVk(&application.engine, font.glyphPositions[glyphIds[i]].advanceWidth);
	};
	textObject->instanceCount = j;
	const VkDeviceSize s = textObject->instanceCount * 4 * sizeof(textTransforms[0]) + textObject->instanceCount * textGlyphIds[0];
	if(frCreateBuffer(&application.engine, s, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &textObject->instanceBuffer, &textObject->instanceBufferMemory) != VK_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	void* data;
	if(application.engine.vkMapMemory(application.engine.device, textObject->instanceBufferMemory, 0, s, 0, &data) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	char* trucData = (char*)data;
	for(uint32_t i = 0; i < application.engine.objects[application.engine.objectCount - 1].instanceCount; ++i)
	{
		memcpy(trucData, textTransforms + 4 * i, sizeof(textTransforms[0]) * 4);
		trucData += sizeof(textTransforms[0]) * 4;
		memcpy(trucData, textGlyphIds + i, sizeof(textGlyphIds[0]));
		trucData += sizeof(textGlyphIds[0]);
	}
	application.engine.vkUnmapMemory(application.engine.device, textObject->instanceBufferMemory);
	frArenaRestore(&application.arena, save);

	frFreeFont(&font);

	// Main loop
	const int exitValue = frRunApplication(&application);

	if(frFinish() != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	return exitValue;
}
