#ifndef FRAUS_VULKAN_UTILS_H
#define FRAUS_VULKAN_UTILS_H

#include "include.h"

FrResult frFindMemoryTypeIndex(FrVulkanData* pVulkanData, uint32_t typeBits, VkMemoryPropertyFlags properties, uint32_t* pIndex);

FrResult frBeginCommandBuffer(FrVulkanData* pVulkanData, VkCommandBuffer* pCommandBuffer);
FrResult frEndCommandBuffer(FrVulkanData* pVulkanData, VkCommandBuffer commandBuffer);

FrResult frCreateBuffer(FrVulkanData* pVulkanData, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* pBuffer, VkDeviceMemory* pBufferMemory);
FrResult frCopyBuffer(FrVulkanData* pVulkanData, VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size);

#endif
