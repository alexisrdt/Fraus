#ifndef FRAUS_VULKAN_OBJECT_H
#define FRAUS_VULKAN_OBJECT_H

#include "include.h"

FrResult frCreateObject(FrVulkanData* pVulaknData, FrVertex* pVertices, uint32_t vertexCount, uint32_t* pIndexes, uint32_t indexCount, FrVulkanObject* pVulkanObject);

#endif
