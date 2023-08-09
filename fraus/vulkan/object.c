#include "object.h"

#include "functions.h"
#include "utils.h"

FrResult frCreateObject(FrVulkanData* pVulaknData, FrVertex* pVertices, uint32_t vertexCount, uint32_t* pIndexes, uint32_t indexCount, FrVulkanObject* pObject)
{
	const VkDeviceSize size = vertexCount * sizeof(FrVertex) + indexCount * sizeof(uint32_t);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	if(frCreateBuffer(pVulaknData, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	void* pData;
	vkMapMemory(pVulaknData->device, stagingBufferMemory, 0, size, 0, &pData);
	memcpy(pData, pVertices, vertexCount * sizeof(FrVertex));
	memcpy((uint8_t*)pData + vertexCount * sizeof(FrVertex), pIndexes, indexCount * sizeof(uint32_t));
	vkUnmapMemory(pVulaknData->device, stagingBufferMemory);

	if(frCreateBuffer(pVulaknData, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &pObject->buffer, &pObject->memory) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	if(frCopyBuffer(pVulaknData, stagingBuffer, pObject->buffer, size) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	vkDestroyBuffer(pVulaknData->device, stagingBuffer, NULL);
	vkFreeMemory(pVulaknData->device, stagingBufferMemory, NULL);

	pObject->vertexCount = vertexCount;
	pObject->indexCount = indexCount;

	return FR_SUCCESS;
}
