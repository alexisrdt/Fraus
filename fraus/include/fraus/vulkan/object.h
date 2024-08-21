#ifndef FRAUS_VULKAN_OBJECT_H
#define FRAUS_VULKAN_OBJECT_H

#include "./include.h"

FrResult frCreateObject(const char* modelPath, uint32_t pipelineIndex, const uint32_t* bindingIndexes);
void frDestroyObject(FrVulkanObject* pObject);

#endif
