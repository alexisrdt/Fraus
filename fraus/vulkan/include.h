#ifndef FRAUS_VULKAN_INCLUDE_H
#define FRAUS_VULKAN_INCLUDE_H

#include <stdbool.h>

#define VK_NO_PROTOTYPES

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

typedef struct FrVec3
{
	float x;
	float y;
	float z;
} FrVec3;

typedef struct FrVertex
{
	FrVec3 position;
	FrVec3 color;
} FrVertex;

typedef struct FrVulkanData
{
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
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence frameInFlightFence;
} FrVulkanData;

#endif
