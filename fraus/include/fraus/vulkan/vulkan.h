#ifndef FRAUS_VULKAN_VULKAN_H
#define FRAUS_VULKAN_VULKAN_H

#include "../../../source/vulkan/functions.h"
#include "./include.h"
#include "./object.h"
#include "./vulkan_utils.h"
#include "../window.h"

FrResult frCreateVulkanData(const char* name, uint32_t version);
FrResult frDestroyVulkanData(void);

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
FrResult frCreateGraphicsPipeline(const FrPipelineCreateInfo* pCreateInfo);
FrResult frCreateUniformBuffer(VkDeviceSize size);
FrResult frCreateStorageBuffer(VkDeviceSize size);
FrResult frSetStorageBufferData(uint32_t storageBufferIndex, const void* data, VkDeviceSize size);
FrResult frDrawFrame(void);

FrResult frRecreateSwapchain(void);

#endif
