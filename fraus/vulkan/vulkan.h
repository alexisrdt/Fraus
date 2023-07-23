#ifndef FRAUS_VULKAN_VULKAN_H
#define FRAUS_VULKAN_VULKAN_H

#include "functions.h"
#include "include.h"
#include "../window.h"

FrResult frCreateInstance(const char* pName, uint32_t version, FrVulkanData* pVulkanData);
FrResult frChoosePhysicalDevice(FrVulkanData* pVulkanData);
FrResult frCreateSurface(FrVulkanData* pVulkanData);
FrResult frCreateDevice(FrVulkanData* pVulkanData);
FrResult frCreateSwapchain(FrVulkanData* pVulkanData);
FrResult frCreateRenderPass(FrVulkanData* pVulkanData);
FrResult frCreateFramebuffers(FrVulkanData* pVulkanData);
FrResult frCreateShaderModule(FrVulkanData* pVulkanData, const char* pPath, VkShaderModule* pShaderModule);
FrResult frCreateGraphicsPipeline(FrVulkanData* pVulkanData);
FrResult frCreateVertexBuffer(FrVulkanData* pVulkanData, FrVertex* pVertices, uint32_t vertexCount);
FrResult frCreateIndexBuffer(FrVulkanData* pVulkanData, uint32_t* pIndexes, uint32_t indexCount);
FrResult frCreateUniformBuffer(FrVulkanData* pVulkanData);
FrResult frCreateTexture(FrVulkanData* pVulkanData, const char* pPath);
FrResult frCreateCommandPool(FrVulkanData* pVulkanData);
FrResult frDrawFrame(FrVulkanData* pVulkanData);

FrResult frRecreateSwapchain(FrVulkanData* pVulkanData);

void frSetUpdateHandler(FrVulkanData* pVulkanData, FrUpdateHandler handler, void* pUserData);

#endif
