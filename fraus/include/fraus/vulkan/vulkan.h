#ifndef FRAUS_VULKAN_VULKAN_H
#define FRAUS_VULKAN_VULKAN_H

#include "./functions.h"
#include "./include.h"
#include "./object.h"
#include "./utils.h"
#include "../window.h"

FrResult frCreateVulkanData(FrVulkanData* pVulkanData, const char* pName, uint32_t version);
FrResult frDestroyVulkanData(FrVulkanData* pVulkanData);

FrResult frCreateInstance(FrVulkanData* pVulkanData, const char* pName, uint32_t version);
FrResult frChoosePhysicalDevice(FrVulkanData* pVulkanData);
FrResult frCreateSurface(FrVulkanData* pVulkanData);
FrResult frCreateDevice(FrVulkanData* pVulkanData);
FrResult frCreateSwapchain(FrVulkanData* pVulkanData);
FrResult frCreateRenderPass(FrVulkanData* pVulkanData);
FrResult frCreateFramebuffers(FrVulkanData* pVulkanData);
FrResult frCreateShaderModule(FrVulkanData* pVulkanData, const char* pPath, VkShaderModule* pShaderModule);
FrResult frCreateGraphicsPipeline(
	FrVulkanData* pVulkanData,
	const char* pVertexShaderPath,
	const char* pFragmentShaderPath,
	VkDescriptorSetLayoutBinding* pDescriptorSetLayoutBindings,
	uint32_t descriptorSetLayoutBindingCount
);
FrResult frCreateUniformBuffer(FrVulkanData* pVulkanData, VkDeviceSize size);
FrResult frCreateSampler(FrVulkanData* pVulkanData);
FrResult frCreateColorImage(FrVulkanData* pVulkanData);
FrResult frCreateDepthImage(FrVulkanData* pVulkanData);
FrResult frCreateCommandPools(FrVulkanData* pVulkanData);
FrResult frDrawFrame(FrVulkanData* pVulkanData);

FrResult frRecreateSwapchain(FrVulkanData* pVulkanData);

void frSetUpdateHandler(FrVulkanData* pVulkanData, FrUpdateHandler handler, void* pUserData);

#endif
