#ifndef FRAUS_VULKAN_UTILS_H
#define FRAUS_VULKAN_UTILS_H

#include "./include.h"

FrResult frFindMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties, uint32_t* pIndex);

FrResult frBeginCommandBuffer(VkCommandBuffer* pCommandBuffer);
FrResult frEndCommandBuffer(VkCommandBuffer commandBuffer);

FrResult frCreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* pBuffer, VkDeviceMemory* pBufferMemory);
FrResult frCopyBuffer(VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size);

FrResult frCreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage* pImage, VkDeviceMemory* pImageMemory);
FrResult frCreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageView* pImageView);

FrResult frCreateTexture(const char* path);

#endif
