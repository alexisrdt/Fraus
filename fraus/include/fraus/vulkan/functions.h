#ifndef FRAUS_VULKAN_FUNCTIONS_H
#define FRAUS_VULKAN_FUNCTIONS_H

#include "./include.h"

/* Macros */

#define FR_DECLARE_PFN(name) \
PFN_##name name;

#define FR_LOAD_GLOBAL_PFN(name) \
name = (PFN_##name)vkGetInstanceProcAddr(NULL, #name); \
if(!name) return FR_ERROR_UNKNOWN;

#define FR_LOAD_INSTANCE_PFN(functions, instance, name) \
functions.name = (PFN_##name)vkGetInstanceProcAddr(instance, #name); \
if(!functions.name) return FR_ERROR_UNKNOWN;

#define FR_LOAD_DEVICE_PFN(functions, device, name) \
functions.name = (PFN_##name)functions.vkGetDeviceProcAddr(device, #name); \
if(!functions.name) return FR_ERROR_UNKNOWN;

/* Function pointers */

extern FR_DECLARE_PFN(vkGetInstanceProcAddr)

extern FR_DECLARE_PFN(vkCreateInstance)
extern FR_DECLARE_PFN(vkEnumerateInstanceLayerProperties)
extern FR_DECLARE_PFN(vkEnumerateInstanceExtensionProperties)

#endif
