#include "../../include/fraus/vulkan/object.h"

#include "../../include/fraus/models/models.h"
#include "./functions.h"
#include "../../include/fraus/vulkan/vulkan_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FR_DEFINE_VECTOR(FrVulkanObject, VulkanObject)

FrResult frCreateObject(const char* modelPath, uint32_t pipelineIndex, const uint32_t* bindingIndexes)
{
	FrVulkanObject object = {
		.pipelineIndex = pipelineIndex
	};

	FrModel model;
	if(frLoadOBJ(modelPath, &model) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	const VkDeviceSize size = model.vertexCount * sizeof(model.vertices[0]) + model.indexCount * sizeof(model.indexes[0]);

	// Vertex / index buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	if(frCreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
	memcpy(data, model.vertices, model.vertexCount * sizeof(model.vertices[0]));
	memcpy((uint8_t*)data + model.vertexCount * sizeof(model.vertices[0]), model.indexes, model.indexCount * sizeof(model.indexes[0]));
	vkUnmapMemory(device, stagingBufferMemory);

	if(frCreateBuffer(
		size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&object.buffer,
		&object.memory) != FR_SUCCESS
	)
	{
		return FR_ERROR_UNKNOWN;
	}

	if(frCopyBuffer(stagingBuffer, object.buffer, size) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);

	object.vertexCount = model.vertexCount;
	object.vertices = model.vertices;
	object.indexCount = model.indexCount;

	// Descriptor pool
	uint32_t uniformBuffersCount = 0;
	uint32_t storageBuffersCount = 0;
	uint32_t texturesCount = 0;
	for(uint32_t descriptorTypeIndex = 0; descriptorTypeIndex < graphicsPipelines.data[pipelineIndex].descriptorTypeCount; ++descriptorTypeIndex)
	{
		switch(graphicsPipelines.data[pipelineIndex].descriptorTypes[descriptorTypeIndex])
		{
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				++texturesCount;
				break;

			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				++uniformBuffersCount;
				break;

			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				++storageBuffersCount;
				break;

			default:
				return FR_ERROR_UNKNOWN;
		}
	}

	VkDescriptorPoolSize* const poolSizes = malloc(3 * sizeof(poolSizes[0]));
	if(!poolSizes)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	uint32_t poolSizeCount = 0;
	if(uniformBuffersCount > 0)
	{
		poolSizes[poolSizeCount++] = (VkDescriptorPoolSize){
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = FR_FRAMES_IN_FLIGHT * uniformBuffersCount
		};
	}
	if(texturesCount > 0)
	{
		poolSizes[poolSizeCount++] = (VkDescriptorPoolSize){
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = FR_FRAMES_IN_FLIGHT * texturesCount
		};
	}
	if(storageBuffersCount > 0)
	{
		poolSizes[poolSizeCount++] = (VkDescriptorPoolSize){
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = FR_FRAMES_IN_FLIGHT * storageBuffersCount
		};
	}

	const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = FR_FRAMES_IN_FLIGHT,
		.poolSizeCount = poolSizeCount,
		.pPoolSizes = poolSizes
	};

	if(vkCreateDescriptorPool(
		device,
		&descriptorPoolCreateInfo,
		NULL,
		&object.descriptorPool
	) != VK_SUCCESS)
	{
		vkDestroyBuffer(device, object.buffer, NULL);
		vkFreeMemory(device, object.memory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	free(poolSizes);

	// Descriptor set
	VkDescriptorSetLayout layouts[FR_FRAMES_IN_FLIGHT];
	for(uint32_t i = 0; i < FR_FRAMES_IN_FLIGHT; i++)
	{
		layouts[i] = graphicsPipelines.data[pipelineIndex].descriptorSetLayout;
	}

	const VkDescriptorSetAllocateInfo descriptorSetsAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = object.descriptorPool,
		.descriptorSetCount = FR_FRAMES_IN_FLIGHT,
		.pSetLayouts = layouts
	};
	if(vkAllocateDescriptorSets(device, &descriptorSetsAllocateInfo, object.descriptorSets) != VK_SUCCESS)
	{
		vkDestroyDescriptorPool(device, object.descriptorPool, NULL);
		vkDestroyBuffer(device, object.buffer, NULL);
		vkFreeMemory(device, object.memory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	VkWriteDescriptorSet* const descriptorWrites = malloc(graphicsPipelines.data[pipelineIndex].descriptorTypeCount * sizeof(descriptorWrites[0]));
	if(!descriptorWrites)
	{
		vkDestroyDescriptorPool(device, object.descriptorPool, NULL);
		vkDestroyBuffer(device, object.buffer, NULL);
		vkFreeMemory(device, object.memory, NULL);
		return FR_ERROR_UNKNOWN;
	}
	for(uint32_t descriptorSetIndex = 0; descriptorSetIndex < FR_FRAMES_IN_FLIGHT; ++descriptorSetIndex)
	{
		for(uint32_t descriptorTypeIndex = 0; descriptorTypeIndex < graphicsPipelines.data[pipelineIndex].descriptorTypeCount; ++descriptorTypeIndex)
		{
			switch(graphicsPipelines.data[pipelineIndex].descriptorTypes[descriptorTypeIndex])
			{
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				{
					VkDescriptorBufferInfo* const pBufferInfo = malloc(sizeof(*pBufferInfo));
					if(!pBufferInfo)
					{
						return FR_ERROR_OUT_OF_HOST_MEMORY;
					}
					pBufferInfo->buffer = uniformBuffers.data[bindingIndexes[descriptorTypeIndex]].buffers[descriptorSetIndex];
					pBufferInfo->offset = 0;
					pBufferInfo->range = uniformBuffers.data[bindingIndexes[descriptorTypeIndex]].buffersSize;

					descriptorWrites[descriptorTypeIndex] = (VkWriteDescriptorSet){
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

				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				{
					VkDescriptorBufferInfo* const pBufferInfo = malloc(sizeof(*pBufferInfo));
					if(!pBufferInfo)
					{
						return FR_ERROR_OUT_OF_HOST_MEMORY;
					}
					pBufferInfo->buffer = storageBuffers.data[bindingIndexes[descriptorTypeIndex]].buffer;
					pBufferInfo->offset = 0;
					pBufferInfo->range = storageBuffers.data[bindingIndexes[descriptorTypeIndex]].bufferSize;

					descriptorWrites[descriptorTypeIndex] = (VkWriteDescriptorSet){
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.dstSet = object.descriptorSets[descriptorSetIndex],
						.dstBinding = descriptorTypeIndex,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						.pBufferInfo = pBufferInfo
					};
					break;
				}

				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				{
					VkDescriptorImageInfo* const pImageInfo = malloc(sizeof(*pImageInfo));
					if(!pImageInfo)
					{
						return FR_ERROR_OUT_OF_HOST_MEMORY;
					}
					pImageInfo->sampler = textureSampler;
					pImageInfo->imageView = textures.data[bindingIndexes[descriptorTypeIndex]].imageView;
					pImageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					descriptorWrites[descriptorTypeIndex] = (VkWriteDescriptorSet){
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
		
		vkUpdateDescriptorSets(device, graphicsPipelines.data[pipelineIndex].descriptorTypeCount, descriptorWrites, 0, NULL);

		for(uint32_t descriptorTypeIndex = 0; descriptorTypeIndex < graphicsPipelines.data[pipelineIndex].descriptorTypeCount; ++descriptorTypeIndex)
		{
			if(descriptorWrites[descriptorTypeIndex].pBufferInfo)
			{
				free((void*)descriptorWrites[descriptorTypeIndex].pBufferInfo);
			}
			if(descriptorWrites[descriptorTypeIndex].pImageInfo)
			{
				free((void*)descriptorWrites[descriptorTypeIndex].pImageInfo);
			}
		}
	}

	free(descriptorWrites);
	free(model.indexes);

	if(frPushBackVulkanObjectVector(&frObjects, object) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

void frDestroyObject(FrVulkanObject* pObject)
{
	free(pObject->vertices);

	vkDestroyDescriptorPool(device, pObject->descriptorPool, NULL);
	vkDestroyBuffer(device, pObject->buffer, NULL);
	vkFreeMemory(device, pObject->memory, NULL);
}
