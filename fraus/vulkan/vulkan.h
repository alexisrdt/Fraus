#ifndef FRAUS_VULKAN_VULKAN_H
#define FRAUS_VULKAN_VULKAN_H

#include "functions.h"
#include "include.h"
#include "../window.h"

FrResult frCreateInstance(const char* pName, uint32_t version, FrVulkanData* pVulkanData);
FrResult frChoosePhysicalDevice(FrVulkanData* pVulkanData);
FrResult frCreateSurface(FrWindow* pWindow, FrVulkanData* pVulkanData);
FrResult frCreateDevice(FrVulkanData* pVulkanData);
FrResult frCreateSwapchain(FrVulkanData* pVulkanData);
FrResult frCreateRenderPass(FrVulkanData* pVulkanData);
FrResult frCreateFramebuffers(FrVulkanData* pVulkanData);
FrResult frCreateCommandPool(FrVulkanData* pVulkanData);
FrResult frDrawFrame(FrVulkanData* pVulkanData);

#endif
