#ifndef FRAUS_VULKAN_OBJECT_H
#define FRAUS_VULKAN_OBJECT_H

#include "include.h"

FrResult frCreateObject(FrVulkanData* pVulkanData, const char* pModelPath, const char* pTexturePath);
void frDestroyObject(FrVulkanData* pVulkanData, FrVulkanObject* pObject);

#endif
