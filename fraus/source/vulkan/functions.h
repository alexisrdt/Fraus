#ifndef FRAUS_VULKAN_FUNCTIONS_H
#define FRAUS_VULKAN_FUNCTIONS_H

#include "fraus/vulkan/include.h"
#include "fraus/utils.h"

// General PFNs
#ifndef NDEBUG
#define FR_DEBUG_GENERAL_PFNS(F) \
	F(vkEnumerateInstanceLayerProperties)
#else
#define FR_DEBUG_GENERAL_PFNS(F)
#endif

#define FR_GENERAL_PFNS(F) \
	FR_DEBUG_GENERAL_PFNS(F) \
	F(vkEnumerateInstanceExtensionProperties) \
	F(vkCreateInstance)

// Instance PFNs
#ifdef VK_USE_PLATFORM_WIN32_KHR
#define FR_PLATFORM_PFNS(F) \
	F(vkCreateWin32SurfaceKHR)
#else
#define FR_PLATFORM_PFNS(F) \
	F(vkCreateXlibSurfaceKHR)
#endif

#ifndef NDEBUG
#define FR_DEBUG_INSTANCE_PFNS(F) \
	F(vkCreateDebugUtilsMessengerEXT) \
	F(vkDestroyDebugUtilsMessengerEXT) \
	F(vkSetDebugUtilsObjectNameEXT)
#else
#define FR_DEBUG_INSTANCE_PFNS(F)
#endif

#define FR_BASE_INSTANCE_PFNS(F) \
	FR_PLATFORM_PFNS(F) \
	F(vkDestroyInstance) \
	F(vkDestroySurfaceKHR) \
	F(vkEnumeratePhysicalDevices) \
	F(vkGetPhysicalDeviceProperties) \
	F(vkGetPhysicalDeviceFeatures) \
	F(vkGetPhysicalDeviceQueueFamilyProperties) \
	F(vkGetPhysicalDeviceMemoryProperties) \
	F(vkGetPhysicalDeviceFormatProperties) \
	F(vkGetPhysicalDeviceSurfaceSupportKHR) \
	F(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
	F(vkGetPhysicalDeviceSurfaceFormatsKHR) \
	F(vkGetPhysicalDeviceSurfacePresentModesKHR) \
	F(vkEnumerateDeviceExtensionProperties) \
	F(vkGetDeviceProcAddr) \
	F(vkCreateDevice)

#define FR_INSTANCE_PFNS(F) \
	FR_DEBUG_INSTANCE_PFNS(F) \
	FR_BASE_INSTANCE_PFNS(F)

// Device PFNs
#define FR_DEVICE_PFNS(F) \
	F(vkDestroyDevice) \
	F(vkGetDeviceQueue) \
	F(vkCreateSwapchainKHR) \
	F(vkDestroySwapchainKHR) \
	F(vkGetSwapchainImagesKHR) \
	F(vkCreateImageView) \
	F(vkDestroyImageView) \
	F(vkCreateRenderPass) \
	F(vkDestroyRenderPass) \
	F(vkCreateFramebuffer) \
	F(vkDestroyFramebuffer) \
	F(vkCreateShaderModule) \
	F(vkDestroyShaderModule) \
	F(vkCreateDescriptorSetLayout) \
	F(vkDestroyDescriptorSetLayout) \
	F(vkCreatePipelineLayout) \
	F(vkDestroyPipelineLayout) \
	F(vkCreateGraphicsPipelines) \
	F(vkDestroyPipeline) \
	F(vkCreateCommandPool) \
	F(vkDestroyCommandPool) \
	F(vkResetCommandPool) \
	F(vkAllocateCommandBuffers) \
	F(vkFreeCommandBuffers) \
	F(vkBeginCommandBuffer) \
	F(vkEndCommandBuffer) \
	F(vkQueueSubmit) \
	F(vkQueuePresentKHR) \
	F(vkAcquireNextImageKHR) \
	F(vkCreateSemaphore) \
	F(vkDestroySemaphore) \
	F(vkCreateFence) \
	F(vkDestroyFence) \
	F(vkResetFences) \
	F(vkWaitForFences) \
	F(vkCreateBuffer) \
	F(vkDestroyBuffer) \
	F(vkGetBufferMemoryRequirements) \
	F(vkBindBufferMemory) \
	F(vkAllocateMemory) \
	F(vkFreeMemory) \
	F(vkMapMemory) \
	F(vkUnmapMemory) \
	F(vkCreateImage) \
	F(vkDestroyImage) \
	F(vkGetImageMemoryRequirements) \
	F(vkCreateSampler) \
	F(vkDestroySampler) \
	F(vkCreateDescriptorPool) \
	F(vkDestroyDescriptorPool) \
	F(vkAllocateDescriptorSets) \
	F(vkUpdateDescriptorSets) \
	F(vkCmdBeginRenderPass) \
	F(vkCmdEndRenderPass) \
	F(vkCmdBindPipeline) \
	F(vkCmdBindVertexBuffers) \
	F(vkCmdBindIndexBuffer) \
	F(vkCmdDrawIndexed) \
	F(vkCmdPipelineBarrier) \
	F(vkCmdSetViewport) \
	F(vkCmdSetScissor) \
	F(vkCmdPushConstants) \
	F(vkCmdBindDescriptorSets) \
	F(vkCmdCopyBuffer) \
	F(vkCmdCopyBufferToImage) \
	F(vkCmdBlitImage) \
	F(vkCmdExecuteCommands) \
	F(vkBindImageMemory) \
	F(vkQueueWaitIdle) \
	F(vkDeviceWaitIdle)

#define FR_PFNS(F) \
	F(vkGetInstanceProcAddr) \
	FR_GENERAL_PFNS(F) \
	FR_INSTANCE_PFNS(F) \
	FR_DEVICE_PFNS(F)

#define FR_DECLARE_EXTERN_PFN(name) \
extern PFN_##name name;

FR_PFNS(FR_DECLARE_EXTERN_PFN)

/*
 * Load global Vulkan functions.
 *
 * Returns:
 * - FR_SUCCESS if everything went well.
 * - FR_ERROR_UNKNOWN if some other error occured.
 */
FrResult frLoadGlobalFunctions(void);

/*
 * Load instance Vulkan functions.
 *
 * Returns:
 * - FR_SUCCESS if everything went well.
 * - FR_ERROR_UNKNOWN if some other error occured.
 */
FrResult frLoadInstanceFunctions(void);

/*
 * Load device Vulkan functions.
 *
 * Returns:
 * - FR_SUCCESS if everything went well.
 * - FR_ERROR_UNKNOWN if some other error occured.
 */
FrResult frLoadDeviceFunctions(void);

#endif
