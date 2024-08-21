#include "./spirv.h"

#include <stdbool.h>
#include <stdlib.h>

#include <spirv-headers/spirv.h>

typedef enum FrBaseType
{
	FrBaseTypeInt,
	FrBaseTypeUint,
	FrBaseTypeFloat,
	FrBaseTypeDouble
} FrBaseType;

typedef struct FrSpvObject
{
	SpvOp objectType;

	union
	{
		// Variable
		struct
		{
			uint32_t size;
			FrBaseType base;
			uint32_t location;
			uint32_t binding;
			SpvStorageClass storageClass;
			bool hasBlockDecoration;
		};

		// Type
		struct
		{
			uint32_t typeSize;
			FrBaseType typeBase;
		};
	};
} FrSpvObject;

static VkFormat frVariableFormat(uint32_t size, FrBaseType base)
{
	switch(base)
	{
		case FrBaseTypeInt:
			switch(size)
			{
				case 4:
					return VK_FORMAT_R32_SINT;
				case 8:
					return VK_FORMAT_R32G32_SINT;
				case 12:
					return VK_FORMAT_R32G32B32_SINT;
				case 16:
					return VK_FORMAT_R32G32B32A32_SINT;
				default:
					break;
			}
			break;

		case FrBaseTypeUint:
			switch(size)
			{
				case 4:
					return VK_FORMAT_R32_UINT;
				case 8:
					return VK_FORMAT_R32G32_UINT;
				case 12:
					return VK_FORMAT_R32G32B32_UINT;
				case 16:
					return VK_FORMAT_R32G32B32A32_UINT;
				default:
					break;
			}
			break;

		case FrBaseTypeFloat:
			switch(size)
			{
				case 4:
					return VK_FORMAT_R32_SFLOAT;
				case 8:
					return VK_FORMAT_R32G32_SFLOAT;
				case 12:
					return VK_FORMAT_R32G32B32_SFLOAT;
				case 16:
					return VK_FORMAT_R32G32B32A32_SFLOAT;
				default:
					break;
			}
			break;

		case FrBaseTypeDouble:
			switch(size)
			{
				case 8:
					return VK_FORMAT_R64_SFLOAT;
				case 16:
					return VK_FORMAT_R64G64_SFLOAT;
				case 24:
					return VK_FORMAT_R64G64B64_SFLOAT;
				case 32:
					return VK_FORMAT_R64G64B64A64_SFLOAT;
				default:
					break;
			}
			break;

		default:
			break;
	}

	return VK_FORMAT_UNDEFINED;
}

static int frCompareShaderVariable(const void* a, const void* b)
{
	const FrShaderVariable* const pA = a;
	const FrShaderVariable* const pB = b;

	if(pA->location < pB->location)
	{
		return -1;
	}
	if(pA->location > pB->location)
	{
		return 1;
	}
	return 0;
}

int frCompareBindings(const void* pAV, const void* pBV)
{
	const VkDescriptorSetLayoutBinding* pA = pAV;
	const VkDescriptorSetLayoutBinding* pB = pBV;

	if(pA->binding < pB->binding)
	{
		return -1;
	}
	if(pA->binding > pB->binding)
	{
		return 1;
	}
	return 0;
}

FrResult frParseSpirv(const uint32_t* code, size_t size, FrShaderInfo* pInfo)
{
	if(!code || size < 5 || !pInfo)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}
	*pInfo = (FrShaderInfo){0};

	const uint32_t idsBound = code[3];
	FrSpvObject* const objects = calloc(idsBound, sizeof(objects[0]));
	if(!objects)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	uint32_t i = 5;
	while(i < size)
	{
		const uint32_t wordCount = code[i] >> SpvWordCountShift;
		const SpvOp opcode = code[i] & SpvOpCodeMask;

		if(i + wordCount > size)
		{
			free(objects);
			return FR_ERROR_CORRUPTED_FILE;
		}

		switch(opcode)
		{
			case SpvOpTypeInt:
			{
				if(wordCount != 4)
				{
					free(objects);
					return FR_ERROR_CORRUPTED_FILE;
				}

				const SpvId typeId = code[i + 1];
				const uint32_t width = code[i + 2];
				const uint32_t signedness = code[i + 3];

				objects[typeId].objectType = opcode;
				objects[typeId].typeSize = width / 8;
				objects[typeId].typeBase = signedness ? FrBaseTypeInt : FrBaseTypeUint;

				break;
			}

			case SpvOpTypeFloat:
			{
				if(wordCount != 3)
				{
					free(objects);
					return FR_ERROR_CORRUPTED_FILE;
				}

				const SpvId typeId = code[i + 1];
				const uint32_t width = code[i + 2];

				objects[typeId].objectType = opcode;
				objects[typeId].typeSize = width / 8;
				objects[typeId].typeBase = width > 32 ? FrBaseTypeDouble : FrBaseTypeFloat;

				break;
			}

			case SpvOpTypeVector:
			{
				if(wordCount != 4)
				{
					free(objects);
					return FR_ERROR_CORRUPTED_FILE;
				}

				const SpvId typeId = code[i + 1];
				const SpvId componentTypeId = code[i + 2];
				const uint32_t componentCount = code[i + 3];

				objects[typeId].objectType = opcode;
				objects[typeId].typeSize = objects[componentTypeId].typeSize * componentCount;
				objects[typeId].typeBase = objects[componentTypeId].typeBase;

				break;
			}

			case SpvOpTypeMatrix:
			{
				if(wordCount != 4)
				{
					free(objects);
					return FR_ERROR_CORRUPTED_FILE;
				}

				const SpvId typeId = code[i + 1];
				const SpvId columnTypeId = code[i + 2];
				const uint32_t columnCount = code[i + 3];

				objects[typeId].objectType = opcode;
				objects[typeId].typeSize = objects[columnTypeId].typeSize * columnCount;
				objects[typeId].typeBase = objects[columnTypeId].typeBase;

				break;
			}

			case SpvOpTypeStruct:
			{
				if(wordCount < 2)
				{
					free(objects);
					return FR_ERROR_CORRUPTED_FILE;
				}

				const SpvId typeId = code[i + 1];

				objects[typeId].objectType = opcode;
				for(uint32_t j = 2; j < wordCount; ++j)
				{
					const SpvId memberTypeId = code[i + j];
					objects[typeId].typeSize += objects[memberTypeId].typeSize;
				}

				break;
			}

			case SpvOpTypePointer:
			{
				if(wordCount != 4)
				{
					free(objects);
					return FR_ERROR_CORRUPTED_FILE;
				}

				const SpvId pointerId = code[i + 1];
				const SpvId typeId = code[i + 3];

				objects[pointerId].objectType = opcode;
				objects[pointerId].typeSize = objects[typeId].typeSize;
				objects[pointerId].typeBase = objects[typeId].typeBase;

				break;
			}

			case SpvOpVariable:
			{
				if(wordCount < 4)
				{
					free(objects);
					return FR_ERROR_CORRUPTED_FILE;
				}

				const SpvId variableId = code[i + 2];
				const SpvId typeId = code[i + 1];
				const SpvStorageClass storageClass = code[i + 3];

				objects[variableId].objectType = opcode;
				objects[variableId].size = objects[typeId].typeSize;
				objects[variableId].base = objects[typeId].typeBase;
				objects[variableId].storageClass = storageClass;

				switch(storageClass)
				{
					case SpvStorageClassInput:
						if(objects[variableId].location)
						{
							++pInfo->inputCount;
						}
						break;

					case SpvStorageClassOutput:
						if(objects[variableId].location)
						{
							++pInfo->outputCount;
						}
						break;

					case SpvStorageClassPushConstant:
						++pInfo->pushConstantCount;
						break;

					default:
						break;
				}

				break;
			}

			case SpvOpDecorate:
			{
				if(wordCount < 4)
				{
					break;
				}

				const SpvId targetId = code[i + 1];
				const SpvDecoration decoration = code[i + 2];

				objects[targetId].objectType = SpvOpVariable;

				switch(decoration)
				{
					case SpvDecorationBinding:
						++pInfo->bindingCount;
						objects[targetId].binding = code[i + 3] + 1;
						break;

					case SpvDecorationLocation:
						objects[targetId].location = code[i + 3] + 1;
						break;

					case SpvDecorationBlock:
						objects[targetId].hasBlockDecoration = true;
						break;

					default:
						break;
				}

				break;
			}

			default:
				break;
		}

		i += wordCount;
	}
	if(i != size)
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	if(pInfo->bindingCount)
	{
		pInfo->bindings = malloc(pInfo->bindingCount * sizeof(pInfo->bindings[0]));
		if(!pInfo->bindings)
		{
			free(objects);
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
	}
	if(pInfo->inputCount)
	{
		pInfo->inputs = malloc(pInfo->inputCount * sizeof(pInfo->inputs[0]));
		if(!pInfo->inputs)
		{
			free(pInfo->bindings);
			free(objects);
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
	}
	if(pInfo->outputCount)
	{
		pInfo->outputs = malloc(pInfo->outputCount * sizeof(pInfo->outputs[0]));
		if(!pInfo->outputs)
		{
			free(pInfo->inputs);
			free(pInfo->bindings);
			free(objects);
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
	}
	if(pInfo->pushConstantCount)
	{
		pInfo->pushConstants = malloc(pInfo->pushConstantCount * sizeof(pInfo->pushConstants[0]));
		if(!pInfo->pushConstants)
		{
			free(pInfo->outputs);
			free(pInfo->inputs);
			free(pInfo->bindings);
			free(objects);
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
	}


	i = idsBound - 1;
	uint32_t bindingCounter = 0;
	uint32_t inputCounter = 0;
	uint32_t outputCounter = 0;
	uint32_t pushConstantCounter = 0;
	uint32_t pushConstantOffset = 0;
	while((bindingCounter < pInfo->bindingCount || inputCounter < pInfo->inputCount || outputCounter < pInfo->outputCount) && i > 0)
	{
		if(objects[i].objectType != SpvOpVariable)
		{
			--i;
			continue;
		}

		if(objects[i].storageClass == SpvStorageClassInput && objects[i].location)
		{
			pInfo->inputs[inputCounter].location = objects[i].location - 1;
			pInfo->inputs[inputCounter].size = objects[i].size;
			pInfo->inputs[inputCounter].format = frVariableFormat(objects[i].size, objects[i].base);
			++inputCounter;
		}
		else if(objects[i].storageClass == SpvStorageClassOutput && objects[i].location)
		{
			pInfo->outputs[outputCounter].location = objects[i].location - 1;
			pInfo->outputs[outputCounter].size = objects[i].size;
			pInfo->outputs[outputCounter].format = frVariableFormat(objects[i].size, objects[i].base);
			++outputCounter;
		}
		else if(objects[i].storageClass == SpvStorageClassPushConstant)
		{
			pInfo->pushConstants[pushConstantCounter].offset = pushConstantOffset;
			pInfo->pushConstants[pushConstantCounter].size = objects[i].size;
			pushConstantOffset += objects[i].size;
			++pushConstantCounter;
		}
		else if(objects[i].binding)
		{
			pInfo->bindings[bindingCounter].binding = objects[i].binding - 1;
			pInfo->bindings[bindingCounter].descriptorCount = 1;
			pInfo->bindings[bindingCounter].stageFlags = 0;
			pInfo->bindings[bindingCounter].pImmutableSamplers = NULL;

			switch(objects[i].storageClass)
			{
				case SpvStorageClassUniformConstant:
					pInfo->bindings[bindingCounter].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					break;

				case SpvStorageClassUniform:
					pInfo->bindings[bindingCounter].descriptorType = objects[i].hasBlockDecoration ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					break;

				case SpvStorageClassStorageBuffer:
					pInfo->bindings[bindingCounter].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					break;
			
				default:
					break;
			}

			++bindingCounter;
		}

		--i;
	}

	free(objects);

	if(pInfo->inputs)
	{
		qsort(pInfo->inputs, pInfo->inputCount, sizeof(pInfo->inputs[0]), frCompareShaderVariable);
	}
	if(pInfo->outputs)
	{
		qsort(pInfo->outputs, pInfo->outputCount, sizeof(pInfo->outputs[0]), frCompareShaderVariable);
	}
	if(pInfo->bindings)
	{
		qsort(pInfo->bindings, pInfo->bindingCount, sizeof(pInfo->bindings[0]), frCompareBindings);
	}

	return FR_SUCCESS;
}
