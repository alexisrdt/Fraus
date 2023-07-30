#ifndef FRAUS_VULKAN_INCLUDE_H
#define FRAUS_VULKAN_INCLUDE_H

#include <stdbool.h>

#define VK_NO_PROTOTYPES

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

#include "../math.h"
#include "../window.h"

typedef struct FrModelViewProjection
{
	float model[16];
	float view[16];
	float projection[16];
} FrModelViewProjection;

typedef struct FrVulkanData FrVulkanData;
typedef void(*FrUpdateHandler)(FrVulkanData* pVulkanData, float elapsed, void* pUserData);

typedef struct FrVulkanObject
{
	VkDeviceMemory memory;
	VkBuffer buffer;
	uint32_t vertexCount;
	uint32_t indexCount;
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
	FrVulkanObject object;
	VkBuffer modelViewProjectionBuffer;
	VkDeviceMemory modelViewProjectionBufferMemory;
	void* pModelViewProjectionData;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	uint32_t textureMipLevels;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampleCountFlagBits msaaSamples;
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	VkSampler textureSampler;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence frameInFlightFence;

	FrUpdateHandler updateHandler;
	void* pUpdateHandlerUserData;
} FrVulkanData;

#endif
