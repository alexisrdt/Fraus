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
	uint32_t indexCount;

	float transformation[16];

	VkDeviceMemory textureMemory;
	VkImage textureImage;
	VkImageView textureImageView;

	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSets[FR_FRAMES_IN_FLIGHT];
} FrVulkanObject;

typedef struct FrVulkanData
{
	VkInstance instance;
#ifndef NDEBUG
	bool debugExtensionAvailable;
	VkDebugUtilsMessengerEXT messenger;
#endif
	FrWindow window;
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
	uint32_t objectCount;
	FrVulkanObject* pObjects;
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	VkSampler textureSampler;
	uint32_t frameInFlightIndex;
	VkCommandPool commandPools[FR_FRAMES_IN_FLIGHT];
	VkCommandBuffer commandBuffers[FR_FRAMES_IN_FLIGHT];
	VkSemaphore imageAvailableSemaphores[FR_FRAMES_IN_FLIGHT];
	VkSemaphore renderFinishedSemaphores[FR_FRAMES_IN_FLIGHT];
	VkFence frameInFlightFences[FR_FRAMES_IN_FLIGHT];

	FrUpdateHandler updateHandler;
	void* pUpdateHandlerUserData;
} FrVulkanData;

#endif
