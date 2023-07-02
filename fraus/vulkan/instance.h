#ifndef FRAUS_VULKAN_INSTANCE_H
#define FRAUS_VULKAN_INSTANCE_H

#include "../window.h"
#include "../utils.h"
#include "include.h"

FrResult frCreateInstance(const char* pName, uint32_t version, FrVulkanData* pVulkanData);
FrResult frChoosePhysicalDevice(FrVulkanData* pVulkanData);
FrResult frCreateSurface(FrWindow* pWindow, FrVulkanData* pVulkanData);
FrResult frCreateDevice(FrVulkanData* pVulkanData);
FrResult frCreateSwapchain(FrVulkanData* pVulkanData);

#endif
