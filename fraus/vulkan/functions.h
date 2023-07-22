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
#ifndef NDEBUG
FR_DECLARE_PFN(vkCreateDebugUtilsMessengerEXT)
FR_DECLARE_PFN(vkDestroyDebugUtilsMessengerEXT)
FR_DECLARE_PFN(vkSetDebugUtilsObjectNameEXT)
#endif
FR_DECLARE_PFN(vkEnumeratePhysicalDevices)
FR_DECLARE_PFN(vkGetPhysicalDeviceQueueFamilyProperties)
FR_DECLARE_PFN(vkGetPhysicalDeviceMemoryProperties)
FR_DECLARE_PFN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
FR_DECLARE_PFN(vkGetPhysicalDeviceSurfaceFormatsKHR)
FR_DECLARE_PFN(vkGetPhysicalDeviceSurfacePresentModesKHR)
FR_DECLARE_PFN(vkGetPhysicalDeviceSurfaceSupportKHR)
#ifdef _WIN32
FR_DECLARE_PFN(vkCreateWin32SurfaceKHR)
#endif
FR_DECLARE_PFN(vkDestroySurfaceKHR)
FR_DECLARE_PFN(vkGetDeviceProcAddr)
FR_DECLARE_PFN(vkEnumerateDeviceLayerProperties)
FR_DECLARE_PFN(vkEnumerateDeviceExtensionProperties)
FR_DECLARE_PFN(vkCreateDevice)

FR_DECLARE_PFN(vkDestroyDevice)
FR_DECLARE_PFN(vkGetDeviceQueue)
FR_DECLARE_PFN(vkCreateSwapchainKHR)
FR_DECLARE_PFN(vkDestroySwapchainKHR)
FR_DECLARE_PFN(vkGetSwapchainImagesKHR)
FR_DECLARE_PFN(vkCreateImageView)
FR_DECLARE_PFN(vkDestroyImageView)
FR_DECLARE_PFN(vkCreateFramebuffer)
FR_DECLARE_PFN(vkDestroyFramebuffer)
FR_DECLARE_PFN(vkCreateRenderPass)
FR_DECLARE_PFN(vkDestroyRenderPass)
FR_DECLARE_PFN(vkCreateDescriptorSetLayout)
FR_DECLARE_PFN(vkDestroyDescriptorSetLayout)
FR_DECLARE_PFN(vkCreateShaderModule)
FR_DECLARE_PFN(vkDestroyShaderModule)
FR_DECLARE_PFN(vkCreatePipelineLayout)
FR_DECLARE_PFN(vkDestroyPipelineLayout)
FR_DECLARE_PFN(vkCreateGraphicsPipelines)
FR_DECLARE_PFN(vkDestroyPipeline)
FR_DECLARE_PFN(vkCreateBuffer)
FR_DECLARE_PFN(vkDestroyBuffer)
FR_DECLARE_PFN(vkAllocateMemory)
FR_DECLARE_PFN(vkGetBufferMemoryRequirements)
FR_DECLARE_PFN(vkFreeMemory)
FR_DECLARE_PFN(vkBindBufferMemory)
FR_DECLARE_PFN(vkMapMemory)
FR_DECLARE_PFN(vkUnmapMemory)
FR_DECLARE_PFN(vkCreateSemaphore)
FR_DECLARE_PFN(vkDestroySemaphore)
FR_DECLARE_PFN(vkCreateFence)
FR_DECLARE_PFN(vkDestroyFence)
FR_DECLARE_PFN(vkWaitForFences)
FR_DECLARE_PFN(vkResetFences)
FR_DECLARE_PFN(vkCreateCommandPool)
FR_DECLARE_PFN(vkDestroyCommandPool)
FR_DECLARE_PFN(vkAllocateCommandBuffers)
FR_DECLARE_PFN(vkResetCommandBuffer)
FR_DECLARE_PFN(vkAcquireNextImageKHR)
FR_DECLARE_PFN(vkQueuePresentKHR)
FR_DECLARE_PFN(vkQueueSubmit)
FR_DECLARE_PFN(vkBeginCommandBuffer)
FR_DECLARE_PFN(vkEndCommandBuffer)
FR_DECLARE_PFN(vkCmdBeginRenderPass)
FR_DECLARE_PFN(vkCmdEndRenderPass)
FR_DECLARE_PFN(vkCmdBindPipeline)
FR_DECLARE_PFN(vkCmdBindVertexBuffers)
FR_DECLARE_PFN(vkCmdSetViewport)
FR_DECLARE_PFN(vkCmdSetScissor)
FR_DECLARE_PFN(vkCmdDraw)
FR_DECLARE_PFN(vkDeviceWaitIdle)

#endif
