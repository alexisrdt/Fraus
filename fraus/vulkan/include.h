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
	uint32_t vertexCount;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	uint32_t indexCount;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence frameInFlightFence;
} FrVulkanData;

#endif
