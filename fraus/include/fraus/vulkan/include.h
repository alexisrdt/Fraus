#ifndef FRAUS_VULKAN_INCLUDE_H
#define FRAUS_VULKAN_INCLUDE_H

// Include Vulkan
#include <vulkan/vulkan.h>

#include "../arena.h"
#include "../math.h"
#include "../window.h"

#define FR_FRAMES_IN_FLIGHT 2

typedef struct FrVulkanObject
{
	VkDeviceMemory memory;
	VkBuffer buffer;
	uint32_t vertexCount;
	FrVertex* vertices;
	uint32_t indexCount;

	float transformation[16];

	uint32_t pipelineIndex;

	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSets[FR_FRAMES_IN_FLIGHT];

	uint32_t instanceCount;
	VkDeviceMemory instanceBufferMemory;
	VkBuffer instanceBuffer;
} FrVulkanObject;

typedef struct FrApplication FrApplication;

typedef struct FrPipeline
{
	bool hasPushConstants;
	VkDescriptorType* descriptorTypes;
	uint32_t descriptorTypeCount;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
} FrPipeline;

typedef struct FrUniformBuffer
{
	VkBuffer buffers[FR_FRAMES_IN_FLIGHT];
	VkDeviceSize buffersSize;
	VkDeviceMemory bufferMemories[FR_FRAMES_IN_FLIGHT];
	void* bufferDatas[FR_FRAMES_IN_FLIGHT];
} FrUniformBuffer;

typedef struct FrStorageBuffer
{
	VkBuffer buffer;
	VkDeviceMemory bufferMemory;
	VkDeviceSize bufferSize;
} FrStorageBuffer;

typedef struct FrTexture
{
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
} FrTexture;

typedef struct FrEngine
{
	FrArena* arena;

	PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

	PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
	PFN_vkGetPhysicalDeviceFormatProperties2 vkGetPhysicalDeviceFormatProperties2;

	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;

	PFN_vkQueueSubmit vkQueueSubmit;
	PFN_vkQueueSubmit2 vkQueueSubmit2;

	PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
	PFN_vkCmdPipelineBarrier2 vkCmdPipelineBarrier2;

	PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
	PFN_vkCmdEndRenderPass vkCmdEndRenderPass;

	PFN_vkCmdBeginRenderPass2 vkCmdBeginRenderPass2;
	PFN_vkCmdEndRenderPass2 vkCmdEndRenderPass2;

	PFN_vkCmdBeginRendering vkCmdBeginRendering;
	PFN_vkCmdEndRendering vkCmdEndRendering;

	PFN_vkCreateFramebuffer vkCreateFramebuffer;
	PFN_vkDestroyFramebuffer vkDestroyFramebuffer;

	PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
	PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;

	PFN_vkCreateImage vkCreateImage;
	PFN_vkDestroyImage vkDestroyImage;

	PFN_vkCreateImageView vkCreateImageView;
	PFN_vkDestroyImageView vkDestroyImageView;

	PFN_vkCreateShaderModule vkCreateShaderModule;
	PFN_vkDestroyShaderModule vkDestroyShaderModule;

	PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
	PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
	PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;

	PFN_vkCreateSemaphore vkCreateSemaphore;
	PFN_vkDestroySemaphore vkDestroySemaphore;

	PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
	PFN_vkFreeCommandBuffers vkFreeCommandBuffers;

	PFN_vkCreateBuffer vkCreateBuffer;
	PFN_vkDestroyBuffer vkDestroyBuffer;
	PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
	PFN_vkBindBufferMemory vkBindBufferMemory;
	PFN_vkAllocateMemory vkAllocateMemory;
	PFN_vkFreeMemory vkFreeMemory;
	PFN_vkMapMemory vkMapMemory;
	PFN_vkUnmapMemory vkUnmapMemory;
	PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
	PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
	PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
	PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
	PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;

	PFN_vkWaitForFences vkWaitForFences;
	PFN_vkResetFences vkResetFences;
	PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
	PFN_vkResetCommandPool vkResetCommandPool;

	PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
	PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
	PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
	PFN_vkCmdBlitImage vkCmdBlitImage;
	PFN_vkBindImageMemory vkBindImageMemory;
	PFN_vkCmdBindPipeline vkCmdBindPipeline;
	PFN_vkCmdSetViewport vkCmdSetViewport;
	PFN_vkCmdSetScissor vkCmdSetScissor;
	PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
	PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
	PFN_vkCmdPushConstants vkCmdPushConstants;
	PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
	PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
	PFN_vkEndCommandBuffer vkEndCommandBuffer;

	PFN_vkQueuePresentKHR vkQueuePresentKHR;

	PFN_vkQueueWaitIdle vkQueueWaitIdle;
	PFN_vkDeviceWaitIdle vkDeviceWaitIdle;

	FrPipeline* graphicsPipelines;
	size_t graphicsPipelineCount;

	FrUniformBuffer* uniformBuffers;
	size_t uniformBufferCount;

	FrStorageBuffer* storageBuffers;
	size_t storageBufferCount;

	FrTexture* textures;
	size_t textureCount;

	FrVulkanObject* objects;
	size_t objectCount;

	uint32_t instanceVersion;
	uint32_t deviceVersion;

	bool hasPhysicalDevice2: 1;
#ifndef NDEBUG
	bool hasDebugUtils: 1;
#endif
	bool hasRenderPass2: 1;
	bool hasDynamicRendering: 1;
	bool hasSynchronization2: 1;

	VkInstance instance;
#ifndef NDEBUG
	VkDebugUtilsMessengerEXT messenger;
#endif
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceMemoryProperties2 memoryProperties;

	VkDevice device;
	uint32_t queueFamily;
	VkQueue queue;

	VkRenderPass renderPass;
	VkFramebuffer* framebuffers;

	VkSwapchainKHR swapchain;
	VkExtent2D swapchainExtent;
	VkFormat swapchainFormat;
	uint32_t swapchainImageCount;
	VkImage* swapchainImages;
	VkImageView* swapchainImageViews;

	VkDeviceSize attachmentsMemorySize;
	VkDeviceMemory attachmentsMemory;
	VkImage colorImage;
	VkImageView colorImageView;
	VkImage depthImage;
	VkImageView depthImageView;

	VkSemaphore imageAvailableSemaphores[FR_FRAMES_IN_FLIGHT];
	VkSemaphore* renderFinishedSemaphores;
	VkFence frameInFlightFences[FR_FRAMES_IN_FLIGHT];

	uint32_t textureMipLevels;
	VkSampleCountFlagBits msaaSamples;

	VkSampler textureSampler;
	float maxSamplerAnisotropy;
	bool samplerAnisotropy;
	bool sampleRateShading;

	VkCommandPool commandPools[FR_FRAMES_IN_FLIGHT];
	VkCommandBuffer commandBuffers[FR_FRAMES_IN_FLIGHT];

	uint32_t frameInFlightIndex;
} FrEngine;

#endif
