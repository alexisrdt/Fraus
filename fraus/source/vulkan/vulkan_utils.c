#include "../../include/fraus/vulkan/vulkan_utils.h"

#include "../../include/fraus/images/images.h"
#include "./functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FrResult frFindMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties, uint32_t* pIndex)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	uint32_t memoryTypeIndex;
	for(memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
	{
		if(
			(typeBits & (1 << memoryTypeIndex)) &&
			(memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & properties) == properties
		)
		{
			break;
		}
	}
	if(memoryTypeIndex >= memoryProperties.memoryTypeCount)
	{
		return FR_ERROR_UNKNOWN;
	}

	*pIndex = memoryTypeIndex;

	return FR_SUCCESS;
}

FrResult frBeginCommandBuffer(VkCommandBuffer* pCommandBuffer)
{
	// Create command buffer
	const VkCommandBufferAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPools[frameInFlightIndex],
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	if(vkAllocateCommandBuffers(device, &allocateInfo, pCommandBuffer) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Begin command buffer
	const VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	if(vkBeginCommandBuffer(*pCommandBuffer, &beginInfo) != VK_SUCCESS)
	{
		vkFreeCommandBuffers(device, commandPools[frameInFlightIndex], 1, pCommandBuffer);
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

FrResult frEndCommandBuffer(VkCommandBuffer commandBuffer)
{
	// End command buffer
	if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		vkFreeCommandBuffers(device, commandPools[frameInFlightIndex], 1, &commandBuffer);
		return FR_ERROR_UNKNOWN;
	}

	// Submit to queue
	const VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};
	if(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		vkFreeCommandBuffers(device, commandPools[frameInFlightIndex], 1, &commandBuffer);
		return FR_ERROR_UNKNOWN;
	}

	// Wait idle
	if(vkQueueWaitIdle(queue) != VK_SUCCESS)
	{
		vkFreeCommandBuffers(device, commandPools[frameInFlightIndex], 1, &commandBuffer);
		return FR_ERROR_UNKNOWN;
	}

	// Free command buffer
	vkFreeCommandBuffers(device, commandPools[frameInFlightIndex], 1, &commandBuffer);

	return FR_SUCCESS;
}

FrResult frCreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* pBuffer, VkDeviceMemory* pBufferMemory)
{
	// Create buffer
	const VkBufferCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	if(vkCreateBuffer(device, &createInfo, NULL, pBuffer) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Memory allocation
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, *pBuffer, &memoryRequirements);

	uint32_t memoryTypeIndex;
	if(frFindMemoryTypeIndex(memoryRequirements.memoryTypeBits, properties, &memoryTypeIndex) != FR_SUCCESS)
	{
		vkDestroyBuffer(device, *pBuffer, NULL);
		return FR_ERROR_UNKNOWN;
	}

	const VkMemoryAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	if(vkAllocateMemory(device, &allocateInfo, NULL, pBufferMemory) != VK_SUCCESS)
	{
		vkDestroyBuffer(device, *pBuffer, NULL);
		return FR_ERROR_UNKNOWN;
	}

	if(vkBindBufferMemory(device, *pBuffer, *pBufferMemory, 0) != VK_SUCCESS)
	{
		vkDestroyBuffer(device, *pBuffer, NULL);
		vkFreeMemory(device, *pBufferMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

FrResult frCopyBuffer(VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size)
{
	// Create command buffer
	VkCommandBuffer commandBuffer;
	if(frBeginCommandBuffer(&commandBuffer) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Copy buffer
	const VkBufferCopy region = {
		.size = size
	};
	vkCmdCopyBuffer(commandBuffer, sourceBuffer, destinationBuffer, 1, &region);

	// End command buffer
	if(frEndCommandBuffer(commandBuffer) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

static FrResult frTransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer;
	if(frBeginCommandBuffer(&commandBuffer) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

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
		vkFreeCommandBuffers(device, commandPools[frameInFlightIndex], 1, &commandBuffer);
		return FR_ERROR_INVALID_ARGUMENT;
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);

	if(frEndCommandBuffer(commandBuffer) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

static FrResult frCopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer;
	if(frBeginCommandBuffer(&commandBuffer) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	const VkBufferImageCopy region = {
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
	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	if(frEndCommandBuffer(commandBuffer) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

FrResult frCreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage* pImage, VkDeviceMemory* pImageMemory)
{
	// Create image
	const VkImageCreateInfo createInfo = {
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
	if(vkCreateImage(device, &createInfo, NULL, pImage) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Memory allocation
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(device, *pImage, &memoryRequirements);

	uint32_t memoryTypeIndex;
	if(frFindMemoryTypeIndex(memoryRequirements.memoryTypeBits, properties, &memoryTypeIndex) != FR_SUCCESS)
	{
		vkDestroyImage(device, *pImage, NULL);
		return FR_ERROR_UNKNOWN;
	}

	const VkMemoryAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	if(vkAllocateMemory(device, &allocateInfo, NULL, pImageMemory) != VK_SUCCESS)
	{
		vkDestroyImage(device, *pImage, NULL);
		return FR_ERROR_UNKNOWN;
	}

	if(vkBindImageMemory(device, *pImage, *pImageMemory, 0) != VK_SUCCESS)
	{
		vkDestroyImage(device, *pImage, NULL);
		vkFreeMemory(device, *pImageMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

FrResult frCreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageView* pImageView)
{
	const VkImageViewCreateInfo createInfo = {
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

	if(vkCreateImageView(device, &createInfo, NULL, pImageView) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

static FrResult frGenerateMipmap(VkImage image, VkFormat format, uint32_t width, uint32_t height, uint32_t mipLevels)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
	if(!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		return FR_ERROR_UNKNOWN;
	}

	VkCommandBuffer commandBuffer;
	if(frBeginCommandBuffer(&commandBuffer) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

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

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

		const VkImageBlit blit = {
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
		vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

		if(mipWidth > 1)
		{
			mipWidth /= 2;
		}
		if(mipHeight > 1)
		{
			mipHeight /= 2;
		}
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

	if(frEndCommandBuffer(commandBuffer) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

FrResult frCreateTexture(const char* path)
{
	if(frPushBackTextureVector(&textures, (FrTexture){0}) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Load image
	FrImage image;
	if(frLoadPNG(path, &image) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	// TODO: convert image type in frLoadPNG with parameter
	if(image.type == FR_RGB)
	{
		uint8_t* const newData = malloc(image.width * image.height * 4);
		if(!newData)
		{
			free(image.data);
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}

		for(size_t i = 0; i < image.width * image.height * FR_RGB; ++i)
		{
			newData[i + i / 3] = image.data[i];
		}
		for(size_t i = 0; i < image.width * image.height; ++i)
		{
			newData[4 * i + 3] = 255;
		}
		image.data = newData;
	}

	// Compute mip levels
	uint32_t maxDimension = image.width > image.height ? image.width : image.height;
	maxDimension = maxDimension > 0 ? maxDimension : 1;
	textureMipLevels = 1;
	while(maxDimension >>= 1)
	{
		++textureMipLevels;
	}

	// Compute size
	const VkDeviceSize size = image.width * image.height * 4;

	// Create staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	if(frCreateBuffer(
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		&stagingBufferMemory
	) != FR_SUCCESS)
	{
		free(image.data);
		return FR_ERROR_UNKNOWN;
	}

	// Upload data to the device
	void* data;
	if(vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data) != VK_SUCCESS)
	{
		free(image.data);
		return FR_ERROR_UNKNOWN;
	}
	memcpy(data, image.data, size);
	vkUnmapMemory(device, stagingBufferMemory);

	// Free image
	free(image.data);

	// Create image
	if(frCreateImage(
		image.width,
		image.height,
		textureMipLevels,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&textures.data[textures.size - 1].image,
		&textures.data[textures.size - 1].imageMemory
	) != FR_SUCCESS)
	{
		vkDestroyBuffer(device, stagingBuffer, NULL);
		vkFreeMemory(device, stagingBufferMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	// Copy data to image
	if(frTransitionImageLayout(textures.data[textures.size - 1].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, textureMipLevels) != FR_SUCCESS)
	{
		vkDestroyImage(device, textures.data[textures.size - 1].image, NULL);
		vkFreeMemory(device, textures.data[textures.size - 1].imageMemory, NULL);
		vkDestroyBuffer(device, stagingBuffer, NULL);
		vkFreeMemory(device, stagingBufferMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	if(frCopyBufferToImage(stagingBuffer, textures.data[textures.size - 1].image, image.width, image.height) != FR_SUCCESS)
	{
		vkDestroyImage(device, textures.data[textures.size - 1].image, NULL);
		vkFreeMemory(device, textures.data[textures.size - 1].imageMemory, NULL);
		vkDestroyBuffer(device, stagingBuffer, NULL);
		vkFreeMemory(device, stagingBufferMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);

	// Mipmap
	if(frGenerateMipmap(textures.data[textures.size - 1].image, VK_FORMAT_R8G8B8A8_SRGB, image.width, image.height, textureMipLevels) != FR_SUCCESS)
	{
		vkDestroyImage(device, textures.data[textures.size - 1].image, NULL);
		vkFreeMemory(device, textures.data[textures.size - 1].imageMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	// If no mipmap
	/* if(frTransitionImageLayout(textures.data[textures.size - 1].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureMipLevels) != FR_SUCCESS)
	{
		vkDestroyImage(device, textures.data[textures.size - 1].image, NULL);
		vkFreeMemory(device, textures.data[textures.size - 1].imageMemory, NULL);
		return FR_ERROR_UNKNOWN;
	} */

	// Create image view
	if(frCreateImageView(textures.data[textures.size - 1].image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, textureMipLevels, &textures.data[textures.size - 1].imageView) != FR_SUCCESS)
	{
		vkDestroyImage(device, textures.data[textures.size - 1].image, NULL);
		vkFreeMemory(device, textures.data[textures.size - 1].imageMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}
