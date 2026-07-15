#ifndef FRAUS_VULKAN_VULKAN_H
#define FRAUS_VULKAN_VULKAN_H

#include "./include.h"
#include "./object.h"
#include "./vulkan_utils.h"
#include "../window.h"

FrResult frInitializeVulkan(void);
FrResult frFinishVulkan(void);

FrResult frCreateVulkanEngine(FrApplication* application, const char* name, uint32_t version);
void frDestroyVulkanEngine(FrEngine* engine);

typedef struct FrPipelineCreateInfo
{
	const char* vertexShaderPath;
	const char* fragmentShaderPath;

	uint32_t vertexInputRateCount;
	const VkVertexInputRate* vertexInputRates;
	const uint32_t* vertexInputStrides;

	bool depthTestDisable: 1;
	bool alphaBlendEnable: 1;
} FrPipelineCreateInfo;
FrResult frReserveGraphicsPipelines(FrEngine* engine, size_t count);
FrResult frCreateGraphicsPipeline(FrEngine* engine, const FrPipelineCreateInfo* createInfo);
FrResult frReserveUniformBuffers(FrEngine* engine, size_t count);
FrResult frCreateUniformBuffer(FrEngine* engine, VkDeviceSize size);
FrResult frReserveStorageBuffers(FrEngine* engine, size_t count);
FrResult frCreateStorageBuffer(FrEngine* engine, VkDeviceSize size);
FrResult frSetStorageBufferData(FrEngine* engine, uint32_t storageBufferIndex, const void* data, VkDeviceSize size);
FrResult frReserveTextures(FrEngine* engine, size_t count);
FrResult frReserveObjects(FrEngine* engine, size_t count);
FrResult frDrawFrame(FrApplication* application);

FrResult frRecreateSwapchain(FrApplication* application);

#endif
