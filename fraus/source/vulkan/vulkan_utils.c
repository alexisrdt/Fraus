#include "../../include/fraus/vulkan/vulkan_utils.h"

#include "../../include/fraus/images/images.h"

#include <stdlib.h>
#include <string.h>

VkResult frFindMemoryTypeIndex(FrEngine* const engine, const uint32_t typeBits, const VkMemoryPropertyFlags properties, uint32_t* const index)
{
	*index = engine->memoryProperties.memoryProperties.memoryTypeCount;
	for(uint32_t memoryTypeIndex = 0; memoryTypeIndex < engine->memoryProperties.memoryProperties.memoryTypeCount; ++memoryTypeIndex)
	{
		if(
			(typeBits & (1 << memoryTypeIndex)) &&
			(engine->memoryProperties.memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & properties) == properties
		)
		{
			if(
				*index == engine->memoryProperties.memoryProperties.memoryTypeCount ||
				engine->memoryProperties.memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags < engine->memoryProperties.memoryProperties.memoryTypes[*index].propertyFlags
			)
			{
				*index = memoryTypeIndex;

				if(engine->memoryProperties.memoryProperties.memoryTypes[*index].propertyFlags == properties)
				{
					break;
				}
			}
		}
	}
	if(*index >= engine->memoryProperties.memoryProperties.memoryTypeCount)
	{
		return VK_ERROR_UNKNOWN;
	}

	return VK_SUCCESS;
}

VkResult frBeginCommandBuffer(FrEngine* const engine, VkCommandBuffer* const commandBuffer)
{
	// Create command buffer
	const VkCommandBufferAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = engine->commandPools[engine->frameInFlightIndex],
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	VkResult result = engine->vkAllocateCommandBuffers(engine->device, &allocateInfo, commandBuffer);
	if(result != VK_SUCCESS)
	{
		return result;
	}

	// Begin command buffer
	const VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	result = engine->vkBeginCommandBuffer(*commandBuffer, &beginInfo);
	if(result != VK_SUCCESS)
	{
		engine->vkFreeCommandBuffers(engine->device, engine->commandPools[engine->frameInFlightIndex], 1, commandBuffer);
	}

	return result;
}

VkResult frEndCommandBuffer(FrEngine* const engine, const VkCommandBuffer commandBuffer)
{
	// End command buffer
	VkResult result = engine->vkEndCommandBuffer(commandBuffer);
	if(result != VK_SUCCESS)
	{
		engine->vkFreeCommandBuffers(engine->device, engine->commandPools[engine->frameInFlightIndex], 1, &commandBuffer);
		return result;
	}

	// Submit to queue
	if(engine->hasSynchronization2)
	{
		const VkCommandBufferSubmitInfo commandBufferInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = commandBuffer
		};
		const VkSubmitInfo2 submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &commandBufferInfo
		};
		result = engine->vkQueueSubmit2(engine->queue, 1, &submitInfo, nullptr);
		if(result != VK_SUCCESS)
		{
			engine->vkFreeCommandBuffers(engine->device, engine->commandPools[engine->frameInFlightIndex], 1, &commandBuffer);
			return result;
		}
	}
	else
	{
		const VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer
		};
		result = engine->vkQueueSubmit(engine->queue, 1, &submitInfo, nullptr);
		if(result != VK_SUCCESS)
		{
			engine->vkFreeCommandBuffers(engine->device, engine->commandPools[engine->frameInFlightIndex], 1, &commandBuffer);
			return result;
		}
	}

	// Wait idle
	result = engine->vkQueueWaitIdle(engine->queue);

	// Free command buffer
	engine->vkFreeCommandBuffers(engine->device, engine->commandPools[engine->frameInFlightIndex], 1, &commandBuffer);

	return result;
}

VkResult frCreateBuffer(FrEngine* const engine, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer* const buffer, VkDeviceMemory* const bufferMemory)
{
	// Create buffer
	const VkBufferCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VkResult result = engine->vkCreateBuffer(engine->device, &createInfo, nullptr, buffer);
	if(result != VK_SUCCESS)
	{
		return result;
	}

	// Memory allocation
	VkMemoryRequirements memoryRequirements;
	engine->vkGetBufferMemoryRequirements(engine->device, *buffer, &memoryRequirements);

	uint32_t memoryTypeIndex;
	result = frFindMemoryTypeIndex(engine, memoryRequirements.memoryTypeBits, properties, &memoryTypeIndex);
	if(result != VK_SUCCESS)
	{
		engine->vkDestroyBuffer(engine->device, *buffer, nullptr);
		return result;
	}

	const VkMemoryAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	result = engine->vkAllocateMemory(engine->device, &allocateInfo, nullptr, bufferMemory);
	if(result != VK_SUCCESS)
	{
		engine->vkDestroyBuffer(engine->device, *buffer, nullptr);
		return result;
	}

	result = engine->vkBindBufferMemory(engine->device, *buffer, *bufferMemory, 0);
	if(result != VK_SUCCESS)
	{
		engine->vkDestroyBuffer(engine->device, *buffer, nullptr);
		engine->vkFreeMemory(engine->device, *bufferMemory, nullptr);
	}

	return result;
}

VkResult frCopyBuffer(FrEngine* const engine, const VkBuffer sourceBuffer, const VkBuffer destinationBuffer, const VkDeviceSize size)
{
	// Create command buffer
	VkCommandBuffer commandBuffer;
	const VkResult result = frBeginCommandBuffer(engine, &commandBuffer);
	if(result != VK_SUCCESS)
	{
		return result;
	}

	// Copy buffer
	const VkBufferCopy region = {
		.size = size
	};
	engine->vkCmdCopyBuffer(commandBuffer, sourceBuffer, destinationBuffer, 1, &region);

	// End command buffer
	return frEndCommandBuffer(engine, commandBuffer);
}

static VkResult frTransitionImageLayout(FrEngine* const engine, const VkImage image, const VkImageLayout oldLayout, const VkImageLayout newLayout, const uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer;
	const VkResult result = frBeginCommandBuffer(engine, &commandBuffer);
	if(result != VK_SUCCESS)
	{
		return result;
	}

	if(engine->hasSynchronization2)
	{
		VkImageMemoryBarrier2 barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
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
			barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

			barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		}
		else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

			barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		}
		else
		{
			engine->vkFreeCommandBuffers(engine->device, engine->commandPools[engine->frameInFlightIndex], 1, &commandBuffer);
			return VK_ERROR_UNKNOWN;
		}

		const VkDependencyInfo dependency = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &barrier
		};

		engine->vkCmdPipelineBarrier2(commandBuffer, &dependency);
	}
	else
	{
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
			engine->vkFreeCommandBuffers(engine->device, engine->commandPools[engine->frameInFlightIndex], 1, &commandBuffer);
			return VK_ERROR_UNKNOWN;
		}

		engine->vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	return frEndCommandBuffer(engine, commandBuffer);
}

static VkResult frCopyBufferToImage(FrEngine* const engine, const VkBuffer buffer, const VkImage image, const uint32_t width, const uint32_t height)
{
	VkCommandBuffer commandBuffer;
	const VkResult result = frBeginCommandBuffer(engine, &commandBuffer);
	if(result != VK_SUCCESS)
	{
		return result;
	}

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
	engine->vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	return frEndCommandBuffer(engine, commandBuffer);
}

VkResult frCreateImage(FrEngine* const engine, const uint32_t width, const uint32_t height, const uint32_t mipLevels, const VkSampleCountFlagBits samples, const VkFormat format, const VkImageTiling tiling, const VkImageUsageFlags usage, const VkMemoryPropertyFlags properties, VkImage* const image, VkDeviceMemory* const imageMemory)
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
	VkResult result = engine->vkCreateImage(engine->device, &createInfo, nullptr, image);
	if(result != VK_SUCCESS)
	{
		return result;
	}

	// Memory allocation
	VkMemoryRequirements memoryRequirements;
	engine->vkGetImageMemoryRequirements(engine->device, *image, &memoryRequirements);

	uint32_t memoryTypeIndex;
	result = frFindMemoryTypeIndex(engine, memoryRequirements.memoryTypeBits, properties, &memoryTypeIndex);
	if(result != VK_SUCCESS)
	{
		engine->vkDestroyImage(engine->device, *image, nullptr);
		return result;
	}

	const VkMemoryAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	result = engine->vkAllocateMemory(engine->device, &allocateInfo, nullptr, imageMemory);
	if(result != VK_SUCCESS)
	{
		engine->vkDestroyImage(engine->device, *image, nullptr);
		return result;
	}

	result = engine->vkBindImageMemory(engine->device, *image, *imageMemory, 0);
	if(result != VK_SUCCESS)
	{
		engine->vkDestroyImage(engine->device, *image, nullptr);
		engine->vkFreeMemory(engine->device, *imageMemory, nullptr);
	}

	return result;
}

VkResult frCreateImageView(FrEngine* const engine, const VkImage image, const VkFormat format, const VkImageAspectFlags aspectFlags, const uint32_t mipLevels, VkImageView* const imageView)
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

	return engine->vkCreateImageView(engine->device, &createInfo, nullptr, imageView);
}

static VkResult frGenerateMipmap(FrEngine* const engine, const VkImage image, const VkFormat format, const uint32_t width, const uint32_t height, const uint32_t mipLevels)
{
	VkFormatProperties2 formatProperties;
	if(engine->hasPhysicalDevice2)
	{
		formatProperties.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
		formatProperties.pNext = nullptr;
		engine->vkGetPhysicalDeviceFormatProperties2(engine->physicalDevice, format, &formatProperties);
	}
	else
	{
		engine->vkGetPhysicalDeviceFormatProperties(engine->physicalDevice, format, &formatProperties.formatProperties);
	}
	if(!(formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		return VK_ERROR_FORMAT_NOT_SUPPORTED;
	}

	VkCommandBuffer commandBuffer;
	const VkResult result = frBeginCommandBuffer(engine, &commandBuffer);
	if(result != VK_SUCCESS)
	{
		return result;
	}

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.levelCount = 1,
		.subresourceRange.layerCount = 1
	};

	VkImageMemoryBarrier2 barrier2 = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.levelCount = 1,
		.subresourceRange.layerCount = 1
	};

	const VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier2
	};

	uint32_t mipWidth = width;
	uint32_t mipHeight = height;
	for(uint32_t i = 1; i < mipLevels; ++i)
	{
		if(engine->hasSynchronization2)
		{
			barrier2.subresourceRange.baseMipLevel = i - 1;
			barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier2.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			barrier2.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			barrier2.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			barrier2.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

			engine->vkCmdPipelineBarrier2(commandBuffer, &dependency);
		}
		else
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			engine->vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}

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
		engine->vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		if(engine->hasSynchronization2)
		{
			barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier2.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			barrier2.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			barrier2.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			barrier2.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

			engine->vkCmdPipelineBarrier2(commandBuffer, &dependency);
		}
		else
		{
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			engine->vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}

		if(mipWidth > 1)
		{
			mipWidth /= 2;
		}
		if(mipHeight > 1)
		{
			mipHeight /= 2;
		}
	}

	if(engine->hasSynchronization2)
	{
		barrier2.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier2.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier2.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		barrier2.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier2.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

		engine->vkCmdPipelineBarrier2(commandBuffer, &dependency);
	}
	else
	{
		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		engine->vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	return frEndCommandBuffer(engine, commandBuffer);
}

VkResult frCreateTexture(FrEngine* const engine, const char* const path)
{
	// Load image
	FrImage image;
	if(frLoadPNG(path, &image) != FR_SUCCESS)
	{
		return VK_ERROR_UNKNOWN;
	}

	// TODO: convert image type in frLoadPNG with parameter
	if(image.type == FR_RGB)
	{
		uint8_t* const newData = malloc(image.width * image.height * 4);
		if(!newData)
		{
			free(image.data);
			return VK_ERROR_OUT_OF_HOST_MEMORY;
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
	engine->textureMipLevels = 1;
	while(maxDimension >>= 1)
	{
		++engine->textureMipLevels;
	}

	// Compute size
	const VkDeviceSize size = image.width * image.height * 4;

	// Create staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VkResult result = frCreateBuffer(
		engine,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		&stagingBufferMemory
	);
	if(result != VK_SUCCESS)
	{
		free(image.data);
		return result;
	}

	// Upload data to the device
	void* data;
	result = engine->vkMapMemory(engine->device, stagingBufferMemory, 0, size, 0, &data);
	if(result != VK_SUCCESS)
	{
		free(image.data);
		return result;
	}
	memcpy(data, image.data, size);
	engine->vkUnmapMemory(engine->device, stagingBufferMemory);

	// Free image
	free(image.data);

	// Create image
	result = frCreateImage(
		engine,
		image.width,
		image.height,
		engine->textureMipLevels,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&engine->textures[engine->textureCount].image,
		&engine->textures[engine->textureCount].imageMemory
	);
	if(result != VK_SUCCESS)
	{
		engine->vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
		engine->vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
		return result;
	}

	// Copy data to image
	result = frTransitionImageLayout(engine, engine->textures[engine->textureCount].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, engine->textureMipLevels);
	if(result != VK_SUCCESS)
	{
		engine->vkDestroyImage(engine->device, engine->textures[engine->textureCount].image, nullptr);
		engine->vkFreeMemory(engine->device, engine->textures[engine->textureCount].imageMemory, nullptr);
		engine->vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
		engine->vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
		return VK_ERROR_UNKNOWN;
	}

	result = frCopyBufferToImage(engine, stagingBuffer, engine->textures[engine->textureCount].image, image.width, image.height);
	if(result != VK_SUCCESS)
	{
		engine->vkDestroyImage(engine->device, engine->textures[engine->textureCount].image, nullptr);
		engine->vkFreeMemory(engine->device, engine->textures[engine->textureCount].imageMemory, nullptr);
		engine->vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
		engine->vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
		return result;
	}

	engine->vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
	engine->vkFreeMemory(engine->device, stagingBufferMemory, nullptr);

	// Mipmap
	result = frGenerateMipmap(engine, engine->textures[engine->textureCount].image, VK_FORMAT_R8G8B8A8_SRGB, image.width, image.height, engine->textureMipLevels);
	if(result != VK_SUCCESS)
	{
		engine->vkDestroyImage(engine->device, engine->textures[engine->textureCount].image, nullptr);
		engine->vkFreeMemory(engine->device, engine->textures[engine->textureCount].imageMemory, nullptr);
		return result;
	}

	// If no mipmap
	/* result = frTransitionImageLayout(engine, engine->textures[engine->textureCount].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, engine->textureMipLevels);
	if(result != VK_SUCCESS)
	{
		engine->vkDestroyImage(engine->device, engine->textures[engine->textureCount].image, nullptr);
		engine->vkFreeMemory(engine->device, engine->textures[engine->textureCount].imageMemory, nullptr);
		return result;
	} */

	// Create image view
	result = frCreateImageView(engine, engine->textures[engine->textureCount].image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, engine->textureMipLevels, &engine->textures[engine->textureCount].imageView);
	if(result != VK_SUCCESS)
	{
		engine->vkDestroyImage(engine->device, engine->textures[engine->textureCount].image, nullptr);
		engine->vkFreeMemory(engine->device, engine->textures[engine->textureCount].imageMemory, nullptr);
	}

	++engine->textureCount;

	return result;
}
