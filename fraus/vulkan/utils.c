#include "utils.h"

#include "../images/images.h"
#include "functions.h"

#include <stdio.h>

FrResult frFindMemoryTypeIndex(FrVulkanData* pVulkanData, uint32_t typeBits, VkMemoryPropertyFlags properties, uint32_t* pIndex)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	pVulkanData->functions.vkGetPhysicalDeviceMemoryProperties(pVulkanData->physicalDevice, &memoryProperties);

	uint32_t memoryTypeIndex;
	for(memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
	{
		if(
			(typeBits & (1 << memoryTypeIndex)) &&
			(memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & properties) == properties
		) break;
	}
	if(memoryTypeIndex >= memoryProperties.memoryTypeCount) return FR_ERROR_UNKNOWN;

	*pIndex = memoryTypeIndex;

	return FR_SUCCESS;
}

FrResult frBeginCommandBuffer(FrVulkanData* pVulkanData, VkCommandBuffer* pCommandBuffer)
{
	// Create command buffer
	VkCommandBufferAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = pVulkanData->pCommandPools[pVulkanData->frameInFlightIndex],
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	if(pVulkanData->functions.vkAllocateCommandBuffers(pVulkanData->device, &allocateInfo, pCommandBuffer) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	// Begin command buffer
	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	if(pVulkanData->functions.vkBeginCommandBuffer(*pCommandBuffer, &beginInfo) != VK_SUCCESS)
	{
		pVulkanData->functions.vkFreeCommandBuffers(pVulkanData->device, pVulkanData->pCommandPools[pVulkanData->frameInFlightIndex], 1, pCommandBuffer);
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

FrResult frEndCommandBuffer(FrVulkanData* pVulkanData, VkCommandBuffer commandBuffer)
{
	// End command buffer
	if(pVulkanData->functions.vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		pVulkanData->functions.vkFreeCommandBuffers(pVulkanData->device, pVulkanData->pCommandPools[pVulkanData->frameInFlightIndex], 1, &commandBuffer);
		return FR_ERROR_UNKNOWN;
	}

	// Submit to queue
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};
	if(pVulkanData->functions.vkQueueSubmit(pVulkanData->queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		pVulkanData->functions.vkFreeCommandBuffers(pVulkanData->device, pVulkanData->pCommandPools[pVulkanData->frameInFlightIndex], 1, &commandBuffer);
		return FR_ERROR_UNKNOWN;
	}

	// Wait idle
	if(pVulkanData->functions.vkQueueWaitIdle(pVulkanData->queue) != VK_SUCCESS)
	{
		pVulkanData->functions.vkFreeCommandBuffers(pVulkanData->device, pVulkanData->pCommandPools[pVulkanData->frameInFlightIndex], 1, &commandBuffer);
		return FR_ERROR_UNKNOWN;
	}

	// Free command buffer
	pVulkanData->functions.vkFreeCommandBuffers(pVulkanData->device, pVulkanData->pCommandPools[pVulkanData->frameInFlightIndex], 1, &commandBuffer);

	return FR_SUCCESS;
}

FrResult frCreateBuffer(FrVulkanData* pVulkanData, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* pBuffer, VkDeviceMemory* pBufferMemory)
{
	// Create buffer
	VkBufferCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	if(pVulkanData->functions.vkCreateBuffer(pVulkanData->device, &createInfo, NULL, pBuffer) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	// Memory allocation
	VkMemoryRequirements memoryRequirements;
	pVulkanData->functions.vkGetBufferMemoryRequirements(pVulkanData->device, *pBuffer, &memoryRequirements);

	uint32_t memoryTypeIndex;
	if(frFindMemoryTypeIndex(pVulkanData, memoryRequirements.memoryTypeBits, properties, &memoryTypeIndex) != FR_SUCCESS)
	{
		pVulkanData->functions.vkDestroyBuffer(pVulkanData->device, *pBuffer, NULL);
		return FR_ERROR_UNKNOWN;
	}

	VkMemoryAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	if(pVulkanData->functions.vkAllocateMemory(pVulkanData->device, &allocateInfo, NULL, pBufferMemory) != VK_SUCCESS)
	{
		pVulkanData->functions.vkDestroyBuffer(pVulkanData->device, *pBuffer, NULL);
		return FR_ERROR_UNKNOWN;
	}

	if(pVulkanData->functions.vkBindBufferMemory(pVulkanData->device, *pBuffer, *pBufferMemory, 0) != VK_SUCCESS)
	{
		pVulkanData->functions.vkDestroyBuffer(pVulkanData->device, *pBuffer, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, *pBufferMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

FrResult frCopyBuffer(FrVulkanData* pVulkanData, VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size)
{
	// Create command buffer
	VkCommandBuffer commandBuffer;
	if(frBeginCommandBuffer(pVulkanData, &commandBuffer) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	// Copy buffer
	VkBufferCopy region = {
		.size = size
	};
	pVulkanData->functions.vkCmdCopyBuffer(commandBuffer, sourceBuffer, destinationBuffer, 1, &region);

	// End command buffer
	if(frEndCommandBuffer(pVulkanData, commandBuffer) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	return FR_SUCCESS;
}

static FrResult frTransitionImageLayout(FrVulkanData* pVulkanData, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer;
	if(frBeginCommandBuffer(pVulkanData, &commandBuffer) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	VkPipelineStageFlags sourceStage, destinationStage;

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = mipLevels,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1
	};

	if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		pVulkanData->functions.vkFreeCommandBuffers(pVulkanData->device, pVulkanData->pCommandPools[pVulkanData->frameInFlightIndex], 1, &commandBuffer);
		return FR_ERROR_INVALID_ARGUMENT;
	}

	pVulkanData->functions.vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);

	if(frEndCommandBuffer(pVulkanData, commandBuffer) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	return FR_SUCCESS;
}

static FrResult frCopyBufferToImage(FrVulkanData* pVulkanData, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer;
	if(frBeginCommandBuffer(pVulkanData, &commandBuffer) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.imageSubresource.mipLevel = 0,
		.imageSubresource.baseArrayLayer = 0,
		.imageSubresource.layerCount = 1,
		.imageOffset = {0, 0, 0},
		.imageExtent = {width, height, 1}
	};
	pVulkanData->functions.vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	if(frEndCommandBuffer(pVulkanData, commandBuffer) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	return FR_SUCCESS;
}

FrResult frCreateImage(FrVulkanData* pVulkanData, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage* pImage, VkDeviceMemory* pImageMemory)
{
	// Create image
	VkImageCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent.width = width,
		.extent.height = height,
		.extent.depth = 1,
		.mipLevels = mipLevels,
		.arrayLayers = 1,
		.samples = samples,
		.tiling = tiling,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	if(pVulkanData->functions.vkCreateImage(pVulkanData->device, &createInfo, NULL, pImage) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	// Memory allocation
	VkMemoryRequirements memoryRequirements;
	pVulkanData->functions.vkGetImageMemoryRequirements(pVulkanData->device, *pImage, &memoryRequirements);

	uint32_t memoryTypeIndex;
	if(frFindMemoryTypeIndex(pVulkanData, memoryRequirements.memoryTypeBits, properties, &memoryTypeIndex) != FR_SUCCESS)
	{
		pVulkanData->functions.vkDestroyImage(pVulkanData->device, *pImage, NULL);
		return FR_ERROR_UNKNOWN;
	}

	VkMemoryAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	if(pVulkanData->functions.vkAllocateMemory(pVulkanData->device, &allocateInfo, NULL, pImageMemory) != VK_SUCCESS)
	{
		pVulkanData->functions.vkDestroyImage(pVulkanData->device, *pImage, NULL);
		return FR_ERROR_UNKNOWN;
	}

	if(pVulkanData->functions.vkBindImageMemory(pVulkanData->device, *pImage, *pImageMemory, 0) != VK_SUCCESS)
	{
		pVulkanData->functions.vkDestroyImage(pVulkanData->device, *pImage, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, *pImageMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

FrResult frCreateImageView(FrVulkanData* pVulkanData, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageView* pImageView)
{
	VkImageViewCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange.aspectMask = aspectFlags,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = mipLevels,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1
	};

	if(pVulkanData->functions.vkCreateImageView(pVulkanData->device, &createInfo, NULL, pImageView) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	return FR_SUCCESS;
}

static FrResult frGenerateMipmap(FrVulkanData* pVulkanData, VkImage image, VkFormat format, uint32_t width, uint32_t height, uint32_t mipLevels)
{
	VkFormatProperties formatProperties;
	pVulkanData->functions.vkGetPhysicalDeviceFormatProperties(pVulkanData->physicalDevice, format, &formatProperties);
	if(!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) return FR_ERROR_UNKNOWN;

	VkCommandBuffer commandBuffer;
	if(frBeginCommandBuffer(pVulkanData, &commandBuffer) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1
	};

	uint32_t mipWidth = width;
	uint32_t mipHeight = height;
	for(uint32_t i = 1; i < mipLevels; ++i)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		pVulkanData->functions.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

		VkImageBlit blit = {
			.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.srcSubresource.mipLevel = i - 1,
			.srcSubresource.baseArrayLayer = 0,
			.srcSubresource.layerCount = 1,
			.srcOffsets[0] = {0, 0, 0},
			.srcOffsets[1] = {mipWidth, mipHeight, 1},
			.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.dstSubresource.mipLevel = i,
			.dstSubresource.baseArrayLayer = 0,
			.dstSubresource.layerCount = 1,
			.dstOffsets[0] = {0, 0, 0},
			.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1}
		};
		pVulkanData->functions.vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		pVulkanData->functions.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	pVulkanData->functions.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

	if(frEndCommandBuffer(pVulkanData, commandBuffer) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	return FR_SUCCESS;
}

FrResult frCreateTexture(FrVulkanData* pVulkanData, const char* pPath, VkDeviceMemory* pImageMemory, VkImage* pImage, VkImageView* pImageView)
{
	// Load image
	FrImage image;
	if(frLoadPNG(pPath, &image) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	// TODO: convert image type in frLoadPNG with parameter
	if(image.type == FR_RGB)
	{
		uint8_t* pNewData = malloc(image.width * image.height * 4);
		if(!pNewData)
		{
			free(image.pData);
			return FR_ERROR_OUT_OF_MEMORY;
		}

		for(size_t i = 0; i < image.width * image.height * FR_RGB; ++i)
		{
			pNewData[i + i / 3] = image.pData[i];
		}
		for(size_t i = 0; i < image.width * image.height; ++i)
		{
			pNewData[4 * i + 3] = 255;
		}
		free(image.pData);
		image.pData = pNewData;
	}

	// Compute mip levels
	uint32_t maxDimension = image.width > image.height ? image.width : image.height;
	maxDimension = maxDimension > 0 ? maxDimension : 1;
	pVulkanData->textureMipLevels = 1;
	while(maxDimension >>= 1) ++pVulkanData->textureMipLevels;

	// Compute size
	VkDeviceSize size = image.width * image.height * 4;

	// Create staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	if(frCreateBuffer(
		pVulkanData,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		&stagingBufferMemory
	) != VK_SUCCESS)
	{
		free(image.pData);
		return FR_ERROR_UNKNOWN;
	}

	// Upload data to the device
	void* pData;
	if(pVulkanData->functions.vkMapMemory(pVulkanData->device, stagingBufferMemory, 0, size, 0, &pData) != VK_SUCCESS)
	{
		free(image.pData);
		return FR_ERROR_UNKNOWN;
	}
	memcpy(pData, image.pData, size);
	pVulkanData->functions.vkUnmapMemory(pVulkanData->device, stagingBufferMemory);

	// Free image
	free(image.pData);

	// Create image
	if(frCreateImage(
		pVulkanData,
		image.width,
		image.height,
		pVulkanData->textureMipLevels,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		pImage,
		pImageMemory
	) != VK_SUCCESS)
	{
		pVulkanData->functions.vkDestroyBuffer(pVulkanData->device, stagingBuffer, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, stagingBufferMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	// Copy data to image
	if(frTransitionImageLayout(pVulkanData, *pImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, pVulkanData->textureMipLevels) != FR_SUCCESS)
	{
		pVulkanData->functions.vkDestroyImage(pVulkanData->device, *pImage, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, *pImageMemory, NULL);
		pVulkanData->functions.vkDestroyBuffer(pVulkanData->device, stagingBuffer, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, stagingBufferMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	if(frCopyBufferToImage(pVulkanData, stagingBuffer, *pImage, image.width, image.height) != FR_SUCCESS)
	{
		pVulkanData->functions.vkDestroyImage(pVulkanData->device, *pImage, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, *pImageMemory, NULL);
		pVulkanData->functions.vkDestroyBuffer(pVulkanData->device, stagingBuffer, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, stagingBufferMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	pVulkanData->functions.vkDestroyBuffer(pVulkanData->device, stagingBuffer, NULL);
	pVulkanData->functions.vkFreeMemory(pVulkanData->device, stagingBufferMemory, NULL);

	// Mipmap
	if(frGenerateMipmap(pVulkanData, *pImage, VK_FORMAT_R8G8B8A8_SRGB, image.width, image.height, pVulkanData->textureMipLevels) != FR_SUCCESS)
	{
		pVulkanData->functions.vkDestroyImage(pVulkanData->device, *pImage, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, *pImageMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	// If no mipmap
	/* if(frTransitionImageLayout(pVulkanData, *pImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pVulkanData->textureMipLevels) != FR_SUCCESS)
	{
		pVulkanData->functions.vkDestroyImage(pVulkanData->device, *pImage, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, *pImageMemory, NULL);
		return FR_ERROR_UNKNOWN;
	} */

	// Create image view
	if(frCreateImageView(pVulkanData, *pImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, pVulkanData->textureMipLevels, pImageView) != FR_SUCCESS)
	{
		pVulkanData->functions.vkDestroyImage(pVulkanData->device, *pImage, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, *pImageMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}
