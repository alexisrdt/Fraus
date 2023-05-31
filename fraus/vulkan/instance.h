#ifndef FRAUS_VULKAN_INSTANCE_H
#define FRAUS_VULKAN_INSTANCE_H

#include "../utils.h"
#include "include.h"

FrResult frCreateInstance(const char* pName, uint32_t version, VkInstance* pInstance);

#endif
