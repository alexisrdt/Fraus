#ifndef FRAUS_VULKAN_FUNCTIONS_H
#define FRAUS_VULKAN_FUNCTIONS_H

#include "include.h"

/* Macros */

#define FR_DECLARE_PFN(name) \
PFN_##name name;

#define FR_LOAD_GLOBAL_PFN(name) \
name = (PFN_##name)vkGetInstanceProcAddr(NULL, #name); \
if(!name) return FR_ERROR_UNKNOWN;

#define FR_LOAD_INSTANCE_PFN(instance, name) \
name = (PFN_##name)vkGetInstanceProcAddr(instance, #name); \
if(!name) return FR_ERROR_UNKNOWN;

#define FR_LOAD_DEVICE_PFN(device, name) \
name = (PFN_##name)vkGetDeviceProcAddr(device, #name); \
if(!name) return FR_ERROR_UNKNOWN;

/* Function pointers */

FR_DECLARE_PFN(vkGetInstanceProcAddr)

FR_DECLARE_PFN(vkCreateInstance)
FR_DECLARE_PFN(vkEnumerateInstanceLayerProperties)
FR_DECLARE_PFN(vkEnumerateInstanceExtensionProperties)

FR_DECLARE_PFN(vkDestroyInstance)
FR_DECLARE_PFN(vkEnumeratePhysicalDevices)
FR_DECLARE_PFN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
FR_DECLARE_PFN(vkGetPhysicalDeviceSurfaceFormatsKHR)
FR_DECLARE_PFN(vkGetPhysicalDeviceSurfacePresentModesKHR)
#ifdef _WIN32
FR_DECLARE_PFN(vkCreateWin32SurfaceKHR)
#endif
FR_DECLARE_PFN(vkDestroySurfaceKHR)
FR_DECLARE_PFN(vkGetDeviceProcAddr)
FR_DECLARE_PFN(vkEnumerateDeviceLayerProperties)
FR_DECLARE_PFN(vkEnumerateDeviceExtensionProperties)
FR_DECLARE_PFN(vkCreateDevice)

FR_DECLARE_PFN(vkDestroyDevice)
FR_DECLARE_PFN(vkCreateSwapchainKHR)
FR_DECLARE_PFN(vkDestroySwapchainKHR)

#endif
