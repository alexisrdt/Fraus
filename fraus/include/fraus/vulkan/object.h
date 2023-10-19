#ifndef FRAUS_VULKAN_OBJECT_H
#define FRAUS_VULKAN_OBJECT_H

#include "./include.h"

FrResult frCreateObject(FrVulkanData* pVulkanData, const char* pModelPath, uint32_t pipelineIndex, uint32_t* pBindingIndexes);
void frDestroyObject(FrVulkanData* pVulkanData, FrVulkanObject* pObject);

#endif
