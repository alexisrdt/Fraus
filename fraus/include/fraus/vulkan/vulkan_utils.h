#ifndef FRAUS_VULKAN_UTILS_H
#define FRAUS_VULKAN_UTILS_H

#include "./include.h"

VkResult frFindMemoryTypeIndex(FrEngine* engine, uint32_t typeBits, VkMemoryPropertyFlags properties, uint32_t* index);

VkResult frBeginCommandBuffer(FrEngine* engine, VkCommandBuffer* commandBuffer);
VkResult frEndCommandBuffer(FrEngine* engine, VkCommandBuffer commandBuffer);

VkResult frCreateBuffer(FrEngine* engine, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
VkResult frCopyBuffer(FrEngine* engine, VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size);

VkResult frCreateImage(FrEngine* engine, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage* image, VkDeviceMemory* imageMemory);
VkResult frCreateImageView(FrEngine* engine, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageView* imageView);

VkResult frCreateTexture(FrEngine* engine, const char* path);

#endif
