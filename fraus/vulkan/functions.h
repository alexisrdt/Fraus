#ifndef FRAUS_VULKAN_FUNCTIONS_H
#define FRAUS_VULKAN_FUNCTIONS_H

#include "include.h"

#define FR_DECLARE_PFN(name) \
PFN_##name name;

#define FR_LOAD_GLOBAL_PFN(name) \
name = (PFN_##name)vkGetInstanceProcAddr(NULL, #name); \
if(!name) return FR_ERROR_UNKNOWN;

#define FR_LOAD_INSTANCE_PFN(instance, name) \
name = (PFN_##name)vkGetInstanceProcAddr(instance, #name); \
if(!name) return FR_ERROR_UNKNOWN;

FR_DECLARE_PFN(vkGetInstanceProcAddr)

FR_DECLARE_PFN(vkCreateInstance)

FR_DECLARE_PFN(vkDestroyInstance) // TODO: array for multiple instances?
FR_DECLARE_PFN(vkEnumeratePhysicalDevices)

#endif
