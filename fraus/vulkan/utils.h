#ifndef FRAUS_VULKAN_UTILS_H
#define FRAUS_VULKAN_UTILS_H

#include "include.h"

FrResult frFindMemoryTypeIndex(FrVulkanData* pVulkanData, uint32_t typeBits, VkMemoryPropertyFlags properties, uint32_t* pIndex);

FrResult frBeginCommandBuffer(FrVulkanData* pVulkanData, VkCommandBuffer* pCommandBuffer);
FrResult frEndCommandBuffer(FrVulkanData* pVulkanData, VkCommandBuffer commandBuffer);

FrResult frCreateBuffer(FrVulkanData* pVulkanData, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* pBuffer, VkDeviceMemory* pBufferMemory);
FrResult frCopyBuffer(FrVulkanData* pVulkanData, VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size);

FrResult frCreateImage(FrVulkanData* pVulkanData, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage* pImage, VkDeviceMemory* pImageMemory);
FrResult frCreateImageView(FrVulkanData* pVulkanData, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageView* pImageView);

FrResult frCreateTexture(FrVulkanData* pVulkanData, const char* pPath, VkDeviceMemory* pImageMemory, VkImage* pImage, VkImageView* pImageView);

#endif
