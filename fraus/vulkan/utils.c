#include "utils.h"

#include "functions.h"

FrResult frFindMemoryTypeIndex(FrVulkanData* pVulkanData, uint32_t typeBits, VkMemoryPropertyFlags properties, uint32_t* pIndex)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(pVulkanData->physicalDevice, &memoryProperties);

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
		.commandPool = pVulkanData->commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	if(vkAllocateCommandBuffers(pVulkanData->device, &allocateInfo, pCommandBuffer) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	// Begin command buffer
	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	if(vkBeginCommandBuffer(*pCommandBuffer, &beginInfo) != VK_SUCCESS)
	{
		vkFreeCommandBuffers(pVulkanData->device, pVulkanData->commandPool, 1, pCommandBuffer);
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

FrResult frEndCommandBuffer(FrVulkanData* pVulkanData, VkCommandBuffer commandBuffer)
{
	// End command buffer
	if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		vkFreeCommandBuffers(pVulkanData->device, pVulkanData->commandPool, 1, &commandBuffer);
		return FR_ERROR_UNKNOWN;
	}

	// Submit to queue
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};
	if(vkQueueSubmit(pVulkanData->queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		vkFreeCommandBuffers(pVulkanData->device, pVulkanData->commandPool, 1, &commandBuffer);
		return FR_ERROR_UNKNOWN;
	}

	// Wait idle
	if(vkQueueWaitIdle(pVulkanData->queue) != VK_SUCCESS)
	{
		vkFreeCommandBuffers(pVulkanData->device, pVulkanData->commandPool, 1, &commandBuffer);
		return FR_ERROR_UNKNOWN;
	}

	// Free command buffer
	vkFreeCommandBuffers(pVulkanData->device, pVulkanData->commandPool, 1, &commandBuffer);

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

	if(vkCreateBuffer(pVulkanData->device, &createInfo, NULL, pBuffer) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	// Memory allocation
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(pVulkanData->device, *pBuffer, &memoryRequirements);

	uint32_t memoryTypeIndex;
	if(frFindMemoryTypeIndex(pVulkanData, memoryRequirements.memoryTypeBits, properties, &memoryTypeIndex) != FR_SUCCESS)
	{
		vkDestroyBuffer(pVulkanData->device, *pBuffer, NULL);
		return FR_ERROR_UNKNOWN;
	}

	VkMemoryAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	if(vkAllocateMemory(pVulkanData->device, &allocateInfo, NULL, pBufferMemory) != VK_SUCCESS)
	{
		vkDestroyBuffer(pVulkanData->device, *pBuffer, NULL);
		return FR_ERROR_UNKNOWN;
	}

	if(vkBindBufferMemory(pVulkanData->device, *pBuffer, *pBufferMemory, 0) != VK_SUCCESS)
	{
		vkDestroyBuffer(pVulkanData->device, *pBuffer, NULL);
		vkFreeMemory(pVulkanData->device, *pBufferMemory, NULL);
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
	vkCmdCopyBuffer(commandBuffer, sourceBuffer, destinationBuffer, 1, &region);

	// End command buffer
	if(frEndCommandBuffer(pVulkanData, commandBuffer) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	return FR_SUCCESS;
}
