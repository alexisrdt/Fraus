#ifndef FRAUS_VULKAN_INCLUDE_H
#define FRAUS_VULKAN_INCLUDE_H

#include <stdbool.h>

// Load Vulkan dynamically
#define VK_NO_PROTOTYPES

// Vulkan platform
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XLIB_KHR
#endif

// Include Vulkan
#include <vulkan/vulkan.h>

#include "../math.h"
#include "../threads.h"
#include "../vector.h"
#include "../window.h"

typedef struct FrViewProjection
{
	float view[16];
	float projection[16];
} FrViewProjection;

typedef struct FrVulkanData FrVulkanData;
typedef void(*FrUpdateHandler)(FrVulkanData* pVulkanData, float elapsed, void* pUserData);

#define FR_FRAMES_IN_FLIGHT 2

typedef struct FrVulkanObject
{
	VkDeviceMemory memory;
	VkBuffer buffer;
	uint32_t vertexCount;
	FrVertex* pVertices;
	uint32_t indexCount;

	float transformation[16];

	VkDeviceMemory textureMemory;
	VkImage textureImage;
	VkImageView textureImageView;

	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSets[FR_FRAMES_IN_FLIGHT];
} FrVulkanObject;

FR_DECLARE_VECTOR(FrVulkanObject, VulkanObject)

#define FR_DECLARE_PFN(name) \
PFN_##name name;

typedef struct FrVulkanFunctions
{
	FR_DECLARE_PFN(vkDestroyInstance)
#ifndef NDEBUG
	FR_DECLARE_PFN(vkCreateDebugUtilsMessengerEXT)
	FR_DECLARE_PFN(vkDestroyDebugUtilsMessengerEXT)
#endif
	FR_DECLARE_PFN(vkEnumeratePhysicalDevices)
	FR_DECLARE_PFN(vkGetPhysicalDeviceProperties)
	FR_DECLARE_PFN(vkGetPhysicalDeviceFeatures)
	FR_DECLARE_PFN(vkGetPhysicalDeviceQueueFamilyProperties)
	FR_DECLARE_PFN(vkGetPhysicalDeviceMemoryProperties)
	FR_DECLARE_PFN(vkGetPhysicalDeviceFormatProperties)
	FR_DECLARE_PFN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
	FR_DECLARE_PFN(vkGetPhysicalDeviceSurfaceFormatsKHR)
	FR_DECLARE_PFN(vkGetPhysicalDeviceSurfacePresentModesKHR)
	FR_DECLARE_PFN(vkGetPhysicalDeviceSurfaceSupportKHR)
#ifdef _WIN32
	FR_DECLARE_PFN(vkCreateWin32SurfaceKHR)
#else
	FR_DECLARE_PFN(vkCreateXlibSurfaceKHR)
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
	FR_DECLARE_PFN(vkCreateImage)
	FR_DECLARE_PFN(vkDestroyImage)
	FR_DECLARE_PFN(vkCreateImageView)
	FR_DECLARE_PFN(vkDestroyImageView)
	FR_DECLARE_PFN(vkCreateSampler)
	FR_DECLARE_PFN(vkDestroySampler)
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
	FR_DECLARE_PFN(vkGetImageMemoryRequirements)
	FR_DECLARE_PFN(vkFreeMemory)
	FR_DECLARE_PFN(vkBindBufferMemory)
	FR_DECLARE_PFN(vkBindImageMemory)
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
	FR_DECLARE_PFN(vkCreateDescriptorPool)
	FR_DECLARE_PFN(vkDestroyDescriptorPool)
	FR_DECLARE_PFN(vkAllocateDescriptorSets)
	FR_DECLARE_PFN(vkUpdateDescriptorSets)
	FR_DECLARE_PFN(vkAllocateCommandBuffers)
	FR_DECLARE_PFN(vkFreeCommandBuffers)
	FR_DECLARE_PFN(vkResetCommandBuffer)
	FR_DECLARE_PFN(vkResetCommandPool)
	FR_DECLARE_PFN(vkAcquireNextImageKHR)
	FR_DECLARE_PFN(vkQueuePresentKHR)
	FR_DECLARE_PFN(vkQueueSubmit)
	FR_DECLARE_PFN(vkQueueWaitIdle)
	FR_DECLARE_PFN(vkBeginCommandBuffer)
	FR_DECLARE_PFN(vkEndCommandBuffer)
	FR_DECLARE_PFN(vkCmdExecuteCommands)
	FR_DECLARE_PFN(vkCmdBeginRenderPass)
	FR_DECLARE_PFN(vkCmdNextSubpass)
	FR_DECLARE_PFN(vkCmdEndRenderPass)
	FR_DECLARE_PFN(vkCmdBindPipeline)
	FR_DECLARE_PFN(vkCmdPipelineBarrier)
	FR_DECLARE_PFN(vkCmdCopyBuffer)
	FR_DECLARE_PFN(vkCmdCopyBufferToImage)
	FR_DECLARE_PFN(vkCmdBlitImage)
	FR_DECLARE_PFN(vkCmdBindVertexBuffers)
	FR_DECLARE_PFN(vkCmdBindIndexBuffer)
	FR_DECLARE_PFN(vkCmdBindDescriptorSets)
	FR_DECLARE_PFN(vkCmdPushConstants)
	FR_DECLARE_PFN(vkCmdSetViewport)
	FR_DECLARE_PFN(vkCmdSetScissor)
	FR_DECLARE_PFN(vkCmdDraw)
	FR_DECLARE_PFN(vkCmdDrawIndexed)
	FR_DECLARE_PFN(vkDeviceWaitIdle)
} FrVulkanFunctions;

typedef struct FrApplication FrApplication;

typedef struct FrThreadData FrThreadData;

typedef struct FrVulkanData
{
	FrApplication* pApplication;

	FrVulkanFunctions functions;

	VkInstance instance;
#ifndef NDEBUG
	bool debugExtensionAvailable;
	VkDebugUtilsMessengerEXT messenger;
#endif
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	uint32_t queueFamily;
	VkDevice device;
	VkQueue queue;
	VkExtent2D extent;
	VkFormat format;
	VkSwapchainKHR swapchain;
	VkImage* pImages;
	VkImageView* pImageViews;
	VkFramebuffer* pFramebuffers;
	uint32_t imageCount;
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkBuffer viewProjectionBuffers[FR_FRAMES_IN_FLIGHT];
	VkDeviceMemory viewProjectionBufferMemories[FR_FRAMES_IN_FLIGHT];
	void* pViewProjectionDatas[FR_FRAMES_IN_FLIGHT];
	uint32_t textureMipLevels;
	VkSampleCountFlagBits msaaSamples;
	FrVulkanObjectVector objects;
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	VkSampler textureSampler;

	uint32_t frameInFlightIndex;
	uint32_t imageIndex;

	bool threadShouldExit;
	uint32_t threadCount;
	FrThread* pThreads;
	FrThreadData* pThreadsData;

	uint32_t frameFinishedThreadCount;

	FrMutex frameBeginMutex;
	FrConditionVariable frameBeginConditionVariable;

	FrMutex frameEndMutex;
	FrConditionVariable frameEndConditionVariable;

	VkCommandPool* pCommandPools;
	VkCommandBuffer pPrimaryCommandBuffers[FR_FRAMES_IN_FLIGHT];
	VkCommandBuffer* pSecondaryCommandBuffers;

	VkSemaphore imageAvailableSemaphores[FR_FRAMES_IN_FLIGHT];
	VkSemaphore renderFinishedSemaphores[FR_FRAMES_IN_FLIGHT];
	VkFence frameInFlightFences[FR_FRAMES_IN_FLIGHT];

	FrUpdateHandler updateHandler;
	void* pUpdateHandlerUserData;
} FrVulkanData;

typedef struct FrThreadData
{
	FrVulkanData* pVulkanData;
	uint32_t threadIndex;
	bool canRun;
} FrThreadData;

#endif
