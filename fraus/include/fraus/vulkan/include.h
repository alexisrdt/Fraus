#ifndef FRAUS_VULKAN_INCLUDE_H
#define FRAUS_VULKAN_INCLUDE_H

#include <stdbool.h>

// Include Vulkan
#include <vulkan/vulkan.h>

#include "../math.h"
#include "../vector.h"
#include "../window.h"

typedef struct FrVulkanData FrVulkanData;

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
} FrVulkanObject;

FR_DECLARE_VECTOR(FrVulkanObject, VulkanObject)

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

FR_DECLARE_VECTOR(FrPipeline, Pipeline)

typedef struct FrUniformBuffer
{
	VkBuffer buffers[FR_FRAMES_IN_FLIGHT];
	VkDeviceSize buffersSize;
	VkDeviceMemory bufferMemories[FR_FRAMES_IN_FLIGHT];
	void* bufferDatas[FR_FRAMES_IN_FLIGHT];
} FrUniformBuffer;

FR_DECLARE_VECTOR(FrUniformBuffer, UniformBuffer)

typedef struct FrStorageBuffer
{
	VkBuffer buffer;
	VkDeviceMemory bufferMemory;
	VkDeviceSize bufferSize;
} FrStorageBuffer;

FR_DECLARE_VECTOR(FrStorageBuffer, StorageBuffer)

typedef struct FrTexture
{
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
} FrTexture;

FR_DECLARE_VECTOR(FrTexture, Texture)

extern VkInstance instance;
#ifndef NDEBUG
extern bool debugExtensionAvailable;
extern VkDebugUtilsMessengerEXT messenger;
#endif
extern VkSurfaceKHR surface;
extern VkPhysicalDevice physicalDevice;
extern VkDevice device;
extern uint32_t queueFamily;
extern VkQueue queue;

extern VkSwapchainKHR swapchain;
extern VkExtent2D swapchainExtent;
extern VkFormat swapchainFormat;
extern uint32_t swapchainImageCount;
extern VkImage* swapchainImages;
extern VkImageView* swapchainImageViews;

extern VkRenderPass renderPass;
extern VkFramebuffer* framebuffers;

extern FrPipelineVector graphicsPipelines;
extern FrUniformBufferVector uniformBuffers;
extern FrStorageBufferVector storageBuffers;
extern uint32_t textureMipLevels;
extern FrTextureVector textures;
extern VkSampleCountFlagBits msaaSamples;
extern FrVulkanObjectVector frObjects;

extern VkSemaphore imageAvailableSemaphores[FR_FRAMES_IN_FLIGHT];
extern VkSemaphore renderFinishedSemaphores[FR_FRAMES_IN_FLIGHT];
extern VkFence frameInFlightFences[FR_FRAMES_IN_FLIGHT];

extern VkCommandPool commandPools[FR_FRAMES_IN_FLIGHT];
extern VkCommandBuffer commandBuffers[FR_FRAMES_IN_FLIGHT];

extern VkImage colorImage;
extern VkDeviceMemory colorImageMemory;
extern VkImageView colorImageView;
extern VkImage depthImage;
extern VkDeviceMemory depthImageMemory;
extern VkImageView depthImageView;
extern VkSampler textureSampler;

extern uint32_t frameInFlightIndex;
extern uint32_t swapchainImageIndex;

extern uint32_t instanceCount;
extern VkDeviceMemory instanceBufferMemory;
extern VkBuffer instanceBuffer;

#endif
