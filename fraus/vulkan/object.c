#include "object.h"

#include "../models/models.h"
#include "functions.h"
#include "utils.h"

#include <stdio.h>

FrResult frCreateObject(FrVulkanData* pVulkanData, const char* pModelPath, const char* pTexturePath)
{
	++pVulkanData->objectCount;
	FrVulkanObject* pNewObjects = realloc(pVulkanData->pObjects, pVulkanData->objectCount * sizeof(pVulkanData->pObjects[0]));
	if(!pNewObjects) return FR_ERROR_OUT_OF_MEMORY;
	pVulkanData->pObjects = pNewObjects;

	FrModel model;
	if(frLoadOBJ(pModelPath, &model) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	const VkDeviceSize size = model.vertexCount * sizeof(model.pVertices[0]) + model.indexCount * sizeof(model.pIndexes[0]);

	// Vertex / index buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	if(frCreateBuffer(pVulkanData, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	void* pData;
	vkMapMemory(pVulkanData->device, stagingBufferMemory, 0, size, 0, &pData);
	memcpy(pData, model.pVertices, model.vertexCount * sizeof(model.pVertices[0]));
	memcpy((uint8_t*)pData + model.vertexCount * sizeof(model.pVertices[0]), model.pIndexes, model.indexCount * sizeof(model.pIndexes[0]));
	vkUnmapMemory(pVulkanData->device, stagingBufferMemory);

	if(frCreateBuffer(
		pVulkanData,
		size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &pVulkanData->pObjects[pVulkanData->objectCount - 1].buffer,
		&pVulkanData->pObjects[pVulkanData->objectCount - 1].memory) != VK_SUCCESS
	) return FR_ERROR_UNKNOWN;

	if(frCopyBuffer(pVulkanData, stagingBuffer, pVulkanData->pObjects[pVulkanData->objectCount - 1].buffer, size) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	vkDestroyBuffer(pVulkanData->device, stagingBuffer, NULL);
	vkFreeMemory(pVulkanData->device, stagingBufferMemory, NULL);

	pVulkanData->pObjects[pVulkanData->objectCount - 1].vertexCount = model.vertexCount;
	pVulkanData->pObjects[pVulkanData->objectCount - 1].indexCount = model.indexCount;

	// Texture
	if(frCreateTexture(
		pVulkanData,
		pTexturePath,
		&pVulkanData->pObjects[pVulkanData->objectCount - 1].textureMemory,
		&pVulkanData->pObjects[pVulkanData->objectCount - 1].textureImage,
		&pVulkanData->pObjects[pVulkanData->objectCount - 1].textureImageView
	) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	// Descriptor pool
	VkDescriptorPoolSize poolSizes[] = {
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = FR_FRAMES_IN_FLIGHT
		},
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = FR_FRAMES_IN_FLIGHT
		}
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = FR_FRAMES_IN_FLIGHT,
		.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]),
		.pPoolSizes = poolSizes
	};

	if(vkCreateDescriptorPool(
		pVulkanData->device,
		&descriptorPoolCreateInfo,
		NULL,
		&pVulkanData->pObjects[pVulkanData->objectCount - 1].descriptorPool
	) != VK_SUCCESS)
	{
		vkDestroyBuffer(pVulkanData->device, pVulkanData->pObjects[pVulkanData->objectCount - 1].buffer, NULL);
		vkFreeMemory(pVulkanData->device, pVulkanData->pObjects[pVulkanData->objectCount - 1].memory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	// Descriptor set
	VkDescriptorSetLayout layouts[FR_FRAMES_IN_FLIGHT];
	for(uint32_t i = 0; i < FR_FRAMES_IN_FLIGHT; i++)
	{
		layouts[i] = pVulkanData->descriptorSetLayout;
	}

	VkDescriptorSetAllocateInfo descriptorSetsAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = pVulkanData->pObjects[pVulkanData->objectCount - 1].descriptorPool,
		.descriptorSetCount = FR_FRAMES_IN_FLIGHT,
		.pSetLayouts = layouts
	};
	if(vkAllocateDescriptorSets(pVulkanData->device, &descriptorSetsAllocateInfo, pVulkanData->pObjects[pVulkanData->objectCount - 1].descriptorSets) != VK_SUCCESS)
	{
		vkDestroyDescriptorPool(pVulkanData->device, pVulkanData->pObjects[pVulkanData->objectCount - 1].descriptorPool, NULL);
		vkDestroyBuffer(pVulkanData->device, pVulkanData->pObjects[pVulkanData->objectCount - 1].buffer, NULL);
		vkFreeMemory(pVulkanData->device, pVulkanData->pObjects[pVulkanData->objectCount - 1].memory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	for(uint32_t descriptorSetIndex = 0; descriptorSetIndex < FR_FRAMES_IN_FLIGHT; ++descriptorSetIndex)
	{
		VkDescriptorBufferInfo bufferInfo = {
			.buffer = pVulkanData->viewProjectionBuffers[descriptorSetIndex],
			.offset = 0,
			.range = sizeof(FrViewProjection)
		};

		VkDescriptorImageInfo imageInfo = {
			.sampler = pVulkanData->textureSampler,
			.imageView = pVulkanData->pObjects[pVulkanData->objectCount - 1].textureImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		VkWriteDescriptorSet descriptorWrites[] = {
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = pVulkanData->pObjects[pVulkanData->objectCount - 1].descriptorSets[descriptorSetIndex],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &bufferInfo
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = pVulkanData->pObjects[pVulkanData->objectCount - 1].descriptorSets[descriptorSetIndex],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfo
			}
		};

		vkUpdateDescriptorSets(pVulkanData->device, sizeof(descriptorWrites) / sizeof(descriptorWrites[0]), descriptorWrites, 0, NULL);
	}

	free(model.pVertices);
	free(model.pIndexes);

	return FR_SUCCESS;
}

void frDestroyObject(FrVulkanData* pVulkanData, FrVulkanObject* pObject)
{
	vkDestroyImageView(pVulkanData->device, pObject->textureImageView, NULL);
	vkDestroyImage(pVulkanData->device, pObject->textureImage, NULL);
	vkFreeMemory(pVulkanData->device, pObject->textureMemory, NULL);

	vkDestroyDescriptorPool(pVulkanData->device, pObject->descriptorPool, NULL);
	vkDestroyBuffer(pVulkanData->device, pObject->buffer, NULL);
	vkFreeMemory(pVulkanData->device, pObject->memory, NULL);
}
