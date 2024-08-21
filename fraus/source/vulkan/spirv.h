#ifndef FRAUS_VULKAN_SPIRV_H
#define FRAUS_VULKAN_SPIRV_H

#include "fraus/utils.h"
#include "fraus/vulkan/include.h"

typedef struct FrShaderVariable
{
	uint32_t location;
	uint32_t size;
	VkFormat format;
} FrShaderVariable;

typedef struct FrShaderInfo
{
	uint32_t inputCount;
	FrShaderVariable* inputs;
	uint32_t outputCount;
	FrShaderVariable* outputs;

	uint32_t bindingCount;
	VkDescriptorSetLayoutBinding* bindings;

	uint32_t pushConstantCount;
	VkPushConstantRange* pushConstants;
} FrShaderInfo;

int frCompareBindings(const void* pAV, const void* pAB);

FrResult frParseSpirv(const uint32_t* code, size_t size, FrShaderInfo* pInfo);

#endif
