#include "./spirv.h"

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

static VkFormat frVariableFormat(const uint32_t size, const FrBaseType base)
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

static int frCompareShaderVariable(const void* const aVoid, const void* const bVoid)
{
	const FrShaderVariable* const a = aVoid;
	const FrShaderVariable* const b = bVoid;

	if(a->location < b->location)
	{
		return -1;
	}
	if(a->location > b->location)
	{
		return 1;
	}
	return 0;
}

int frCompareBindings(const void* const aVoid, const void* const bVoid)
{
	const VkDescriptorSetLayoutBinding* const a = aVoid;
	const VkDescriptorSetLayoutBinding* const b = bVoid;

	if(a->binding < b->binding)
	{
		return -1;
	}
	if(a->binding > b->binding)
	{
		return 1;
	}
	return 0;
}

FrResult frParseSpirv(const uint32_t* const code, const size_t size, FrShaderInfo* const info)
{
	*info = (FrShaderInfo){};

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
							++info->inputCount;
						}
						break;

					case SpvStorageClassOutput:
						if(objects[variableId].location)
						{
							++info->outputCount;
						}
						break;

					case SpvStorageClassPushConstant:
						++info->pushConstantCount;
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
						++info->bindingCount;
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

	if(info->bindingCount)
	{
		info->bindings = malloc(info->bindingCount * sizeof(info->bindings[0]));
		if(!info->bindings)
		{
			free(objects);
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
	}
	if(info->inputCount)
	{
		info->inputs = malloc(info->inputCount * sizeof(info->inputs[0]));
		if(!info->inputs)
		{
			free(info->bindings);
			free(objects);
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
	}
	if(info->outputCount)
	{
		info->outputs = malloc(info->outputCount * sizeof(info->outputs[0]));
		if(!info->outputs)
		{
			free(info->inputs);
			free(info->bindings);
			free(objects);
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
	}
	if(info->pushConstantCount)
	{
		info->pushConstants = malloc(info->pushConstantCount * sizeof(info->pushConstants[0]));
		if(!info->pushConstants)
		{
			free(info->outputs);
			free(info->inputs);
			free(info->bindings);
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
	while((bindingCounter < info->bindingCount || inputCounter < info->inputCount || outputCounter < info->outputCount) && i > 0)
	{
		if(objects[i].objectType != SpvOpVariable)
		{
			--i;
			continue;
		}

		if(objects[i].storageClass == SpvStorageClassInput && objects[i].location)
		{
			info->inputs[inputCounter].location = objects[i].location - 1;
			info->inputs[inputCounter].size = objects[i].size;
			info->inputs[inputCounter].format = frVariableFormat(objects[i].size, objects[i].base);
			++inputCounter;
		}
		else if(objects[i].storageClass == SpvStorageClassOutput && objects[i].location)
		{
			info->outputs[outputCounter].location = objects[i].location - 1;
			info->outputs[outputCounter].size = objects[i].size;
			info->outputs[outputCounter].format = frVariableFormat(objects[i].size, objects[i].base);
			++outputCounter;
		}
		else if(objects[i].storageClass == SpvStorageClassPushConstant)
		{
			info->pushConstants[pushConstantCounter].offset = pushConstantOffset;
			info->pushConstants[pushConstantCounter].size = objects[i].size;
			pushConstantOffset += objects[i].size;
			++pushConstantCounter;
		}
		else if(objects[i].binding)
		{
			info->bindings[bindingCounter].binding = objects[i].binding - 1;
			info->bindings[bindingCounter].descriptorCount = 1;
			info->bindings[bindingCounter].stageFlags = 0;
			info->bindings[bindingCounter].pImmutableSamplers = nullptr;

			switch(objects[i].storageClass)
			{
				case SpvStorageClassUniformConstant:
					info->bindings[bindingCounter].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					break;

				case SpvStorageClassUniform:
					info->bindings[bindingCounter].descriptorType = objects[i].hasBlockDecoration ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					break;

				case SpvStorageClassStorageBuffer:
					info->bindings[bindingCounter].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					break;
			
				default:
					break;
			}

			++bindingCounter;
		}

		--i;
	}

	free(objects);

	if(info->inputs)
	{
		qsort(info->inputs, info->inputCount, sizeof(info->inputs[0]), frCompareShaderVariable);
	}
	if(info->outputs)
	{
		qsort(info->outputs, info->outputCount, sizeof(info->outputs[0]), frCompareShaderVariable);
	}
	if(info->bindings)
	{
		qsort(info->bindings, info->bindingCount, sizeof(info->bindings[0]), frCompareBindings);
	}

	return FR_SUCCESS;
}
