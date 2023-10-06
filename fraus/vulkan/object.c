#include "object.h"

#include "../models/models.h"
#include "functions.h"
#include "utils.h"

#include <stdio.h>

FR_DEFINE_VECTOR(FrVulkanObject, VulkanObject)

FrResult frCreateObject(FrVulkanData* pVulkanData, const char* pModelPath, uint32_t pipelineIndex, uint32_t* pBindingIndexes)
{
	FrVulkanObject object = {
		.pipelineIndex = pipelineIndex
	};

	FrModel model;
	if(frLoadOBJ(pModelPath, &model) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	const VkDeviceSize size = model.vertexCount * sizeof(model.pVertices[0]) + model.indexCount * sizeof(model.pIndexes[0]);

	// Vertex / index buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	if(frCreateBuffer(pVulkanData, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	void* pData;
	pVulkanData->functions.vkMapMemory(pVulkanData->device, stagingBufferMemory, 0, size, 0, &pData);
	memcpy(pData, model.pVertices, model.vertexCount * sizeof(model.pVertices[0]));
	memcpy((uint8_t*)pData + model.vertexCount * sizeof(model.pVertices[0]), model.pIndexes, model.indexCount * sizeof(model.pIndexes[0]));
	pVulkanData->functions.vkUnmapMemory(pVulkanData->device, stagingBufferMemory);

	if(frCreateBuffer(
		pVulkanData,
		size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &object.buffer,
		&object.memory) != VK_SUCCESS
	) return FR_ERROR_UNKNOWN;

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		char name[64];
		if(snprintf(name, sizeof(name), "Vertex / index buffer %zu", pVulkanData->objects.size) < 0) return FR_ERROR_UNKNOWN;
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_BUFFER,
			.objectHandle = (uint64_t)object.buffer,
			.pObjectName = name
		};
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

		if(snprintf(name, sizeof(name), "Vertex / index buffer memory %zu", pVulkanData->objects.size) < 0) return FR_ERROR_UNKNOWN;
		nameInfo.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
		nameInfo.objectHandle = (uint64_t)object.memory;
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != FR_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	if(frCopyBuffer(pVulkanData, stagingBuffer, object.buffer, size) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	pVulkanData->functions.vkDestroyBuffer(pVulkanData->device, stagingBuffer, NULL);
	pVulkanData->functions.vkFreeMemory(pVulkanData->device, stagingBufferMemory, NULL);

	object.vertexCount = model.vertexCount;
	object.pVertices = model.pVertices;
	object.indexCount = model.indexCount;

	// Descriptor pool
	uint32_t uniformBuffersCount = 0;
	uint32_t texturesCount = 0;
	for(uint32_t descriptorTypeIndex = 0; descriptorTypeIndex < pVulkanData->graphicsPipelines.pData[pipelineIndex].descriptorTypeCount; ++descriptorTypeIndex)
	{
		switch(pVulkanData->graphicsPipelines.pData[pipelineIndex].pDescriptorTypes[descriptorTypeIndex])
		{
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				++uniformBuffersCount;
				break;

			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				++texturesCount;
				break;

			default:
				return FR_ERROR_UNKNOWN;
		}
	}
	VkDescriptorPoolSize poolSizes[] = {
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = FR_FRAMES_IN_FLIGHT * uniformBuffersCount
		},
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = FR_FRAMES_IN_FLIGHT * texturesCount
		}
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = FR_FRAMES_IN_FLIGHT,
		.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]),
		.pPoolSizes = poolSizes
	};

	if(pVulkanData->functions.vkCreateDescriptorPool(
		pVulkanData->device,
		&descriptorPoolCreateInfo,
		NULL,
		&object.descriptorPool
	) != VK_SUCCESS)
	{
		pVulkanData->functions.vkDestroyBuffer(pVulkanData->device, object.buffer, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, object.memory, NULL);
		return FR_ERROR_UNKNOWN;
	}

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		char name[64];
		if(snprintf(name, sizeof(name), "Descriptor pool %zu", pVulkanData->objects.size) < 0) return FR_ERROR_UNKNOWN;
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL,
			.objectHandle = (uint64_t)object.descriptorPool,
			.pObjectName = name
		};
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != FR_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	// Descriptor set
	VkDescriptorSetLayout layouts[FR_FRAMES_IN_FLIGHT];
	for(uint32_t i = 0; i < FR_FRAMES_IN_FLIGHT; i++)
	{
		layouts[i] = pVulkanData->graphicsPipelines.pData[pipelineIndex].descriptorSetLayout;
	}

	VkDescriptorSetAllocateInfo descriptorSetsAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = object.descriptorPool,
		.descriptorSetCount = FR_FRAMES_IN_FLIGHT,
		.pSetLayouts = layouts
	};
	if(pVulkanData->functions.vkAllocateDescriptorSets(pVulkanData->device, &descriptorSetsAllocateInfo, object.descriptorSets) != VK_SUCCESS)
	{
		pVulkanData->functions.vkDestroyDescriptorPool(pVulkanData->device, object.descriptorPool, NULL);
		pVulkanData->functions.vkDestroyBuffer(pVulkanData->device, object.buffer, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, object.memory, NULL);
		return FR_ERROR_UNKNOWN;
	}

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		char name[64];
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET,
			.pObjectName = name
		};
		for(uint32_t descriptorSetIndex = 0; descriptorSetIndex < FR_FRAMES_IN_FLIGHT; ++descriptorSetIndex)
		{
			if(snprintf(name, sizeof(name), "Descriptor set %zu %u", pVulkanData->objects.size, descriptorSetIndex) < 0) return FR_ERROR_UNKNOWN;
			nameInfo.objectHandle = (uint64_t)object.descriptorSets[descriptorSetIndex];

			if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != FR_SUCCESS) return FR_ERROR_UNKNOWN;
		}
	}
#endif

	VkWriteDescriptorSet* pDescriptorWrites = malloc(pVulkanData->graphicsPipelines.pData[pipelineIndex].descriptorTypeCount * sizeof(VkWriteDescriptorSet));
	if(!pDescriptorWrites)
	{
		pVulkanData->functions.vkDestroyDescriptorPool(pVulkanData->device, object.descriptorPool, NULL);
		pVulkanData->functions.vkDestroyBuffer(pVulkanData->device, object.buffer, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, object.memory, NULL);
		return FR_ERROR_UNKNOWN;
	}
	for(uint32_t descriptorSetIndex = 0; descriptorSetIndex < FR_FRAMES_IN_FLIGHT; ++descriptorSetIndex)
	{
		for(uint32_t descriptorTypeIndex = 0; descriptorTypeIndex < pVulkanData->graphicsPipelines.pData[pipelineIndex].descriptorTypeCount; ++descriptorTypeIndex)
		{
			switch(pVulkanData->graphicsPipelines.pData[pipelineIndex].pDescriptorTypes[descriptorTypeIndex])
			{
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				{
					VkDescriptorBufferInfo* pBufferInfo = malloc(sizeof(VkDescriptorBufferInfo));
					if(!pBufferInfo) return FR_ERROR_OUT_OF_MEMORY;
					pBufferInfo->buffer = pVulkanData->uniformBuffers.pData[pBindingIndexes[descriptorTypeIndex]].buffers[descriptorSetIndex];
					pBufferInfo->offset = 0;
					pBufferInfo->range = pVulkanData->uniformBuffers.pData[pBindingIndexes[descriptorTypeIndex]].buffersSize;

					pDescriptorWrites[descriptorTypeIndex] = (VkWriteDescriptorSet){
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.dstSet = object.descriptorSets[descriptorSetIndex],
						.dstBinding = descriptorTypeIndex,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						.pBufferInfo = pBufferInfo
					};
					break;
				}

				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				{
					VkDescriptorImageInfo* pImageInfo = malloc(sizeof(VkDescriptorImageInfo));
					if(!pImageInfo) return FR_ERROR_OUT_OF_MEMORY;
					pImageInfo->sampler = pVulkanData->textureSampler;
					pImageInfo->imageView = pVulkanData->textures.pData[pBindingIndexes[descriptorTypeIndex]].imageView;
					pImageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					pDescriptorWrites[descriptorTypeIndex] = (VkWriteDescriptorSet){
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.dstSet = object.descriptorSets[descriptorSetIndex],
						.dstBinding = descriptorTypeIndex,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						.pImageInfo = pImageInfo
					};
					break;
				}

				default:
					return FR_ERROR_UNKNOWN;
			}
		}
		
		pVulkanData->functions.vkUpdateDescriptorSets(pVulkanData->device, pVulkanData->graphicsPipelines.pData[pipelineIndex].descriptorTypeCount, pDescriptorWrites, 0, NULL);

		for(uint32_t descriptorTypeIndex = 0; descriptorTypeIndex < pVulkanData->graphicsPipelines.pData[pipelineIndex].descriptorTypeCount; ++descriptorTypeIndex)
		{
			if(pDescriptorWrites[descriptorTypeIndex].pBufferInfo) free((void*)pDescriptorWrites[descriptorTypeIndex].pBufferInfo);
			if(pDescriptorWrites[descriptorTypeIndex].pImageInfo) free((void*)pDescriptorWrites[descriptorTypeIndex].pImageInfo);
		}
	}

	free(pDescriptorWrites);
	free(model.pIndexes);

	if(frPushBackVulkanObjectVector(&pVulkanData->objects, object) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	return FR_SUCCESS;
}

void frDestroyObject(FrVulkanData* pVulkanData, FrVulkanObject* pObject)
{
	free(pObject->pVertices);

	pVulkanData->functions.vkDestroyDescriptorPool(pVulkanData->device, pObject->descriptorPool, NULL);
	pVulkanData->functions.vkDestroyBuffer(pVulkanData->device, pObject->buffer, NULL);
	pVulkanData->functions.vkFreeMemory(pVulkanData->device, pObject->memory, NULL);
}
