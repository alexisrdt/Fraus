#include "../../include/fraus/vulkan/object.h"

#include "../../include/fraus/models/models.h"
#include "../../include/fraus/vulkan/vulkan_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FrResult frCreateObject(FrEngine* const engine, const char* const modelPath, const uint32_t pipelineIndex, const uint32_t* const bindingIndexes)
{
	FrVulkanObject object = {
		.pipelineIndex = pipelineIndex,
		.instanceCount = 1
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

	if(frCreateBuffer(engine, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	void* data;
	engine->vkMapMemory(engine->device, stagingBufferMemory, 0, size, 0, &data);
	memcpy(data, model.vertices, model.vertexCount * sizeof(model.vertices[0]));
	memcpy((uint8_t*)data + model.vertexCount * sizeof(model.vertices[0]), model.indexes, model.indexCount * sizeof(model.indexes[0]));
	engine->vkUnmapMemory(engine->device, stagingBufferMemory);

	if(frCreateBuffer(
		engine,
		size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&object.buffer,
		&object.memory) != VK_SUCCESS
	)
	{
		return FR_ERROR_UNKNOWN;
	}

	if(frCopyBuffer(engine, stagingBuffer, object.buffer, size) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	engine->vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
	engine->vkFreeMemory(engine->device, stagingBufferMemory, nullptr);

	object.vertexCount = model.vertexCount;
	object.vertices = model.vertices;
	object.indexCount = model.indexCount;

	// Descriptor pool
	uint32_t uniformBuffersCount = 0;
	uint32_t storageBuffersCount = 0;
	uint32_t texturesCount = 0;
	for(uint32_t descriptorTypeIndex = 0; descriptorTypeIndex < engine->graphicsPipelines[pipelineIndex].descriptorTypeCount; ++descriptorTypeIndex)
	{
		switch(engine->graphicsPipelines[pipelineIndex].descriptorTypes[descriptorTypeIndex])
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

	VkDescriptorPoolSize poolSizes[3];

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

	if(engine->vkCreateDescriptorPool(
		engine->device,
		&descriptorPoolCreateInfo,
		nullptr,
		&object.descriptorPool
	) != VK_SUCCESS)
	{
		engine->vkDestroyBuffer(engine->device, object.buffer, nullptr);
		engine->vkFreeMemory(engine->device, object.memory, nullptr);
		return FR_ERROR_UNKNOWN;
	}

	// Descriptor set
	VkDescriptorSetLayout layouts[FR_FRAMES_IN_FLIGHT];
	for(uint32_t i = 0; i < FR_FRAMES_IN_FLIGHT; i++)
	{
		layouts[i] = engine->graphicsPipelines[pipelineIndex].descriptorSetLayout;
	}

	const VkDescriptorSetAllocateInfo descriptorSetsAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = object.descriptorPool,
		.descriptorSetCount = FR_FRAMES_IN_FLIGHT,
		.pSetLayouts = layouts
	};
	if(engine->vkAllocateDescriptorSets(engine->device, &descriptorSetsAllocateInfo, object.descriptorSets) != VK_SUCCESS)
	{
		engine->vkDestroyDescriptorPool(engine->device, object.descriptorPool, nullptr);
		engine->vkDestroyBuffer(engine->device, object.buffer, nullptr);
		engine->vkFreeMemory(engine->device, object.memory, nullptr);
		return FR_ERROR_UNKNOWN;
	}

	VkWriteDescriptorSet* const descriptorWrites = malloc(engine->graphicsPipelines[pipelineIndex].descriptorTypeCount * sizeof(descriptorWrites[0]));
	if(!descriptorWrites)
	{
		engine->vkDestroyDescriptorPool(engine->device, object.descriptorPool, nullptr);
		engine->vkDestroyBuffer(engine->device, object.buffer, nullptr);
		engine->vkFreeMemory(engine->device, object.memory, nullptr);
		return FR_ERROR_UNKNOWN;
	}
	for(uint32_t descriptorSetIndex = 0; descriptorSetIndex < FR_FRAMES_IN_FLIGHT; ++descriptorSetIndex)
	{
		for(uint32_t descriptorTypeIndex = 0; descriptorTypeIndex < engine->graphicsPipelines[pipelineIndex].descriptorTypeCount; ++descriptorTypeIndex)
		{
			switch(engine->graphicsPipelines[pipelineIndex].descriptorTypes[descriptorTypeIndex])
			{
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				{
					VkDescriptorBufferInfo* const bufferInfo = malloc(sizeof(*bufferInfo));
					if(!bufferInfo)
					{
						return FR_ERROR_OUT_OF_HOST_MEMORY;
					}
					bufferInfo->buffer = engine->uniformBuffers[bindingIndexes[descriptorTypeIndex]].buffers[descriptorSetIndex];
					bufferInfo->offset = 0;
					bufferInfo->range = engine->uniformBuffers[bindingIndexes[descriptorTypeIndex]].buffersSize;

					descriptorWrites[descriptorTypeIndex] = (VkWriteDescriptorSet){
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.dstSet = object.descriptorSets[descriptorSetIndex],
						.dstBinding = descriptorTypeIndex,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						.pBufferInfo = bufferInfo
					};
					break;
				}

				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				{
					VkDescriptorBufferInfo* const bufferInfo = malloc(sizeof(*bufferInfo));
					if(!bufferInfo)
					{
						return FR_ERROR_OUT_OF_HOST_MEMORY;
					}
					bufferInfo->buffer = engine->storageBuffers[bindingIndexes[descriptorTypeIndex]].buffer;
					bufferInfo->offset = 0;
					bufferInfo->range = engine->storageBuffers[bindingIndexes[descriptorTypeIndex]].bufferSize;

					descriptorWrites[descriptorTypeIndex] = (VkWriteDescriptorSet){
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.dstSet = object.descriptorSets[descriptorSetIndex],
						.dstBinding = descriptorTypeIndex,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						.pBufferInfo = bufferInfo
					};
					break;
				}

				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				{
					VkDescriptorImageInfo* const imageInfo = malloc(sizeof(*imageInfo));
					if(!imageInfo)
					{
						return FR_ERROR_OUT_OF_HOST_MEMORY;
					}
					imageInfo->sampler = engine->textureSampler;
					imageInfo->imageView = engine->textures[bindingIndexes[descriptorTypeIndex]].imageView;
					imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					descriptorWrites[descriptorTypeIndex] = (VkWriteDescriptorSet){
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.dstSet = object.descriptorSets[descriptorSetIndex],
						.dstBinding = descriptorTypeIndex,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						.pImageInfo = imageInfo
					};
					break;
				}

				default:
					return FR_ERROR_UNKNOWN;
			}
		}
		
		engine->vkUpdateDescriptorSets(engine->device, engine->graphicsPipelines[pipelineIndex].descriptorTypeCount, descriptorWrites, 0, nullptr);

		for(uint32_t descriptorTypeIndex = 0; descriptorTypeIndex < engine->graphicsPipelines[pipelineIndex].descriptorTypeCount; ++descriptorTypeIndex)
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

	engine->objects[engine->objectCount] = object;
	++engine->objectCount;

	return FR_SUCCESS;
}

void frDestroyObject(FrEngine* const engine, FrVulkanObject* const object)
{
	free(object->vertices);

	engine->vkDestroyBuffer(engine->device, object->instanceBuffer, nullptr);
	engine->vkFreeMemory(engine->device, object->instanceBufferMemory, nullptr);

	engine->vkDestroyDescriptorPool(engine->device, object->descriptorPool, nullptr);
	engine->vkDestroyBuffer(engine->device, object->buffer, nullptr);
	engine->vkFreeMemory(engine->device, object->memory, nullptr);
}
