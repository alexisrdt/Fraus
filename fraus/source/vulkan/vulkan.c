#include "../../include/fraus/vulkan/vulkan.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <spirv-headers/spirv.h>

#include "../../include/fraus/fraus.h"
#include "./spirv.h"

FR_DEFINE_VECTOR(FrPipeline, Pipeline)
FR_DEFINE_VECTOR(FrUniformBuffer, UniformBuffer)
FR_DEFINE_VECTOR(FrStorageBuffer, StorageBuffer)
FR_DEFINE_VECTOR(FrTexture, Texture)

VkInstance instance;
#ifndef NDEBUG
bool debugExtensionAvailable;
VkDebugUtilsMessengerEXT messenger;
#endif
VkSurfaceKHR surface;
VkPhysicalDevice physicalDevice;
VkDevice device;
uint32_t queueFamily;
VkQueue queue;

VkSwapchainKHR swapchain;
VkExtent2D swapchainExtent;
VkFormat swapchainFormat;
uint32_t swapchainImageCount;
VkImage* swapchainImages;
VkImageView* swapchainImageViews;

VkRenderPass renderPass;
VkFramebuffer* framebuffers;

FrPipelineVector graphicsPipelines;
FrUniformBufferVector uniformBuffers;
FrStorageBufferVector storageBuffers;
uint32_t textureMipLevels;
FrTextureVector textures;
VkSampleCountFlagBits msaaSamples;
FrVulkanObjectVector frObjects;

VkSemaphore imageAvailableSemaphores[FR_FRAMES_IN_FLIGHT];
VkSemaphore renderFinishedSemaphores[FR_FRAMES_IN_FLIGHT];
VkFence frameInFlightFences[FR_FRAMES_IN_FLIGHT];

VkCommandPool commandPools[FR_FRAMES_IN_FLIGHT];
VkCommandBuffer commandBuffers[FR_FRAMES_IN_FLIGHT];

VkImage colorImage;
VkDeviceMemory colorImageMemory;
VkImageView colorImageView;
VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;
VkSampler textureSampler;

uint32_t frameInFlightIndex;
uint32_t swapchainImageIndex;

uint32_t instanceCount;
VkDeviceMemory instanceBufferMemory;
VkBuffer instanceBuffer;

static FrResult frCreateInstance(const char* name, uint32_t version);
static FrResult frChoosePhysicalDevice(void);
static FrResult frCreateSurface(void);
static FrResult frCreateDevice(void);
static FrResult frCreateSwapchain(void);
static FrResult frCreateRenderPass(void);
static FrResult frCreateFramebuffers(void);
static FrResult frCreateShaderModule(const char* path, VkShaderModule* pShaderModule, FrShaderInfo* pInfo);
static FrResult frCreateSampler(void);
static FrResult frCreateColorImage(void);
static FrResult frCreateDepthImage(void);
static FrResult frCreateCommandPools(void);

FrResult frCreateVulkanData(const char* name, uint32_t version)
{
	assert(name != NULL);

	swapchain = VK_NULL_HANDLE;
	swapchainImages = NULL;
	swapchainImageViews = NULL;
	framebuffers = NULL;

	if(frCreateVulkanObjectVector(&frObjects) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frCreatePipelineVector(&graphicsPipelines) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frCreateUniformBufferVector(&uniformBuffers) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frCreateStorageBufferVector(&storageBuffers) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frCreateTextureVector(&textures) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	if(frCreateInstance(name, version) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frCreateSurface() != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frChoosePhysicalDevice() != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frCreateDevice() != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frCreateSwapchain() != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frCreateRenderPass() != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frCreateColorImage() != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frCreateDepthImage() != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frCreateFramebuffers() != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frCreateCommandPools() != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if(frCreateSampler() != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	// Camera transform buffer
	if(frCreateUniformBuffer(sizeof(float[16])) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	return FR_SUCCESS;
}

FrResult frDestroyVulkanData(void)
{
	vkDestroyBuffer(device, instanceBuffer, NULL);
	vkFreeMemory(device, instanceBufferMemory, NULL);

	for(uint32_t i = 0; i < FR_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroyCommandPool(device, commandPools[i], NULL);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], NULL);
		vkDestroySemaphore(device, renderFinishedSemaphores[i], NULL);
		vkDestroyFence(device, frameInFlightFences[i], NULL);
	}
	vkDestroySampler(device, textureSampler, NULL);
	
	for(uint32_t textureIndex = 0; textureIndex < textures.size; ++textureIndex)
	{
		vkDestroyImageView(device, textures.data[textureIndex].imageView, NULL);
		vkDestroyImage(device, textures.data[textureIndex].image, NULL);
		vkFreeMemory(device, textures.data[textureIndex].imageMemory, NULL);
	}
	frDestroyTextureVector(&textures);

	for(uint32_t storageBufferIndex = 0; storageBufferIndex < storageBuffers.size; ++storageBufferIndex)
	{
		vkDestroyBuffer(device, storageBuffers.data[storageBufferIndex].buffer, NULL);
		vkFreeMemory(device, storageBuffers.data[storageBufferIndex].bufferMemory, NULL);
	}
	frDestroyStorageBufferVector(&storageBuffers);

	for(uint32_t uniformBufferIndex = 0; uniformBufferIndex < uniformBuffers.size; ++uniformBufferIndex)
	{
		for(uint32_t frameIndex = 0; frameIndex < FR_FRAMES_IN_FLIGHT; ++frameIndex)
		{
			vkDestroyBuffer(device, uniformBuffers.data[uniformBufferIndex].buffers[frameIndex], NULL);
			vkFreeMemory(device, uniformBuffers.data[uniformBufferIndex].bufferMemories[frameIndex], NULL);
		}
	}
	frDestroyUniformBufferVector(&uniformBuffers);

	for(uint32_t objectIndex = 0; objectIndex < frObjects.size; ++objectIndex)
	{
		frDestroyObject(&frObjects.data[objectIndex]);
	}
	frDestroyVulkanObjectVector(&frObjects);

	for(uint32_t pipelineIndex = 0; pipelineIndex < graphicsPipelines.size; ++pipelineIndex)
	{
		vkDestroyPipeline(device, graphicsPipelines.data[pipelineIndex].pipeline, NULL);
		vkDestroyPipelineLayout(device, graphicsPipelines.data[pipelineIndex].pipelineLayout, NULL);
		vkDestroyDescriptorSetLayout(device, graphicsPipelines.data[pipelineIndex].descriptorSetLayout, NULL);
		free(graphicsPipelines.data[pipelineIndex].descriptorTypes);
	}
	frDestroyPipelineVector(&graphicsPipelines);

	vkDestroyRenderPass(device, renderPass, NULL);
	vkDestroyImageView(device, depthImageView, NULL);
	vkDestroyImage(device, depthImage, NULL);
	vkFreeMemory(device, depthImageMemory, NULL);
	vkDestroyImageView(device, colorImageView, NULL);
	vkDestroyImage(device, colorImage, NULL);
	vkFreeMemory(device, colorImageMemory, NULL);
	for(uint32_t i = 0; i < swapchainImageCount; ++i)
	{
		vkDestroyFramebuffer(device, framebuffers[i], NULL);
		vkDestroyImageView(device, swapchainImageViews[i], NULL);
	}
	free(framebuffers);
	free(swapchainImageViews);
	free(swapchainImages);
	vkDestroySwapchainKHR(device, swapchain, NULL);
	vkDestroyDevice(device, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
#ifndef NDEBUG
	if(debugExtensionAvailable)
	{
		vkDestroyDebugUtilsMessengerEXT(instance, messenger, NULL);
	}
#endif
	vkDestroyInstance(instance, NULL);

	return FR_SUCCESS;
}

#ifndef NDEBUG
#define FR_VK_LAYER_KHRONOS_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

/*
 * Check if required layers are available.
 *
 * Parameters:
 * - pAvailable: Pointer to store whether layers are available.
 * 
 * Returns:
 * - FR_SUCCESS: No error occurred.
 * - FR_ERROR_OUT_OF_HOST_MEMORY: Out of host memory.
 * - FR_ERROR_OUT_OF_DEVICE_MEMORY: Out of device memory.
 * - FR_ERROR_UNKNOWN: Unknown error.
 */
static FrResult frValidationLayersAvailable(bool* pAvailable)
{
	// Check argument
	assert(pAvailable != NULL);

	// Get available layers
	uint32_t availableLayerCount;
	VkResult result = vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);
	if(result != VK_SUCCESS)
	{
		switch(result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				return FR_ERROR_OUT_OF_HOST_MEMORY;

			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				return FR_ERROR_OUT_OF_DEVICE_MEMORY;

			default:
				return FR_ERROR_UNKNOWN;
		}
	}
	VkLayerProperties* const availableLayers = malloc(availableLayerCount * sizeof(availableLayers[0]));
	if(!availableLayers)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	result = vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers);
	if(result != VK_SUCCESS)
	{
		free(availableLayers);
		switch(result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				return FR_ERROR_OUT_OF_HOST_MEMORY;

			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				return FR_ERROR_OUT_OF_DEVICE_MEMORY;

			default:
				return FR_ERROR_UNKNOWN;
		}
	}

	*pAvailable = false;
	for(uint32_t availableLayerIndex = 0; availableLayerIndex < availableLayerCount; ++availableLayerIndex)
	{
		if(strcmp(availableLayers[availableLayerIndex].layerName, FR_VK_LAYER_KHRONOS_VALIDATION_LAYER_NAME) == 0)
		{
			*pAvailable = true;
			break;
		}
	}

	free(availableLayers);

	return FR_SUCCESS;
}
#endif

/*
 * Check that required extensions are available
 * - pAvailableExtensions: available extensions
 * - availableExtensionCount: number of available extensions
 * - ppExtensions: required extensions
 * - extensionCount: number of required extensions
 * - requiredExtensionCount: number of required extensions that are not optional
 * - pOptionalExtensionAvaialble: array of booleans to store whether optional extensions are available
 */
static uint32_t frExtensionsAvailable(
	const VkExtensionProperties* availableExtensions,
	uint32_t availableExtensionCount,
	const char* const* extensions,
	uint32_t extensionCount,
	uint32_t requiredExtensionCount,
	bool* optionalExtensionAvailable
)
{
	uint32_t finalCount = 0;
	uint32_t availableExtensionIndex;
	for(uint32_t extensionIndex = 0; extensionIndex < extensionCount; ++extensionIndex)
	{
		for(availableExtensionIndex = 0; availableExtensionIndex < availableExtensionCount; ++availableExtensionIndex)
		{
			if(strcmp(extensions[extensionIndex], availableExtensions[availableExtensionIndex].extensionName) == 0)
			{
				if(extensionIndex >= requiredExtensionCount)
				{
					optionalExtensionAvailable[extensionIndex - requiredExtensionCount] = true;
				}

				break;
			}
		}

		if(availableExtensionIndex == availableExtensionCount)
		{
			if(extensionIndex < requiredExtensionCount)
			{
				return 0;
			}

			optionalExtensionAvailable[extensionIndex - requiredExtensionCount] = false;
		}
		else
		{
			++finalCount;
		}
	}

	return finalCount;
}

#ifndef NDEBUG
/*
 * Debug messenger callback
 * - messageSeverity: message severity
 * - messageTypes: message types
 * - pCallbackData: message data
 * - pUserData: user data
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL frMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
#ifdef _WIN32
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	GetConsoleScreenBufferInfo(console, &consoleInfo);

	// Change console color to identify severity
	switch(messageSeverity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			SetConsoleTextAttribute(console, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			SetConsoleTextAttribute(console, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_INTENSITY);
			break;

		default:
			break;
	}
#else
	switch(messageSeverity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			printf("\x1b[34m");
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			printf("\x1b[32m");
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			printf("\x1b[33m");
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			printf("\x1b[31m");
			break;

		default:
			break;
	}
#endif

	// vkCreateInstance / vkDestroyInstance messenger or global messenger
	printf("[%s|", pUserData ? "G" : "I");

	// Message type
	if(messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)     printf("G");
	if(messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)  printf("V");
	if(messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) printf("P");

	// Message severity
	printf("|");
	switch(messageSeverity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			printf("V");
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			printf("I");
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			printf("W");
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			printf("E");
			break;

		default:
			printf("?");
			break;
	}

	// Object names
	for(uint32_t i = 0; i < pCallbackData->objectCount; ++i)
	{
		printf("|%s", pCallbackData->pObjects[i].pObjectName);
	}

	// Message
	printf("] %s\n\n", pCallbackData->pMessage);

#ifdef _WIN32
	// Reset console color
	SetConsoleTextAttribute(console, consoleInfo.wAttributes);
#else
	printf("\x1b[0m");
#endif

	return VK_FALSE;
}
#endif

static FrResult frCreateInstance(const char* name, uint32_t version)
{
#ifndef NDEBUG
	// Check if validation layers are available
	const char* layers[] = {FR_VK_LAYER_KHRONOS_VALIDATION_LAYER_NAME};
	const uint32_t layerCount = FR_LEN(layers);

	bool validationLayersAvailable;
	FrResult result = frValidationLayersAvailable(&validationLayersAvailable);
	if(result != FR_SUCCESS)
	{
		return result;
	}
	if(!validationLayersAvailable)
	{
		printf("Validation layers not found.\n");
	}
#endif

	uint32_t availableExtensionCount;
	if(vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, NULL) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	VkExtensionProperties* availableExtensions = malloc(availableExtensionCount * sizeof(availableExtensions[0]));
	if(!availableExtensions)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	if(vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, availableExtensions) != VK_SUCCESS)
	{
		free(availableExtensions);
		return FR_ERROR_UNKNOWN;
	}

	const char* extensions[] = {
		// Required
		VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
		// Optional
#ifndef NDEBUG
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
	};
	const uint32_t extensionCount = FR_LEN(extensions);
	const uint32_t requiredExtensionCount = 2;
	bool optionalExtensionsAvailable[1];

	const uint32_t finalExtensionCount = frExtensionsAvailable(
		availableExtensions,
		availableExtensionCount,
		extensions,
		extensionCount,
		requiredExtensionCount,
		optionalExtensionsAvailable
	);
	free(availableExtensions);
	if(!finalExtensionCount)
	{
		return FR_ERROR_UNKNOWN;
	}


#ifndef NDEBUG
	debugExtensionAvailable = optionalExtensionsAvailable[extensionCount - requiredExtensionCount - 1];
#endif

#ifndef NDEBUG
	if(!debugExtensionAvailable && validationLayersAvailable)
	{
		if(vkEnumerateInstanceExtensionProperties(FR_VK_LAYER_KHRONOS_VALIDATION_LAYER_NAME, &availableExtensionCount, NULL) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
		availableExtensions = malloc(availableExtensionCount * sizeof(availableExtensions[0]));
		if(!availableExtensions)
		{
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
		if(vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, availableExtensions) != VK_SUCCESS)
		{
			free(availableExtensions);
			return FR_ERROR_UNKNOWN;
		}

		debugExtensionAvailable = frExtensionsAvailable(
			availableExtensions,
			availableExtensionCount,
			extensions + extensionCount - 1,
			1,
			1,
			NULL
		);

		free(availableExtensions);
	}

	if(!debugExtensionAvailable)
	{
		printf("Debug extension not found.\n");
	}
#endif

	const char** finalExtensions = malloc(finalExtensionCount * sizeof(finalExtensions[0]));
	if(!finalExtensions)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	for(uint32_t i = 0, optionalIndex = requiredExtensionCount; i < extensionCount; ++i)
	{
		if(i < requiredExtensionCount)
		{
			finalExtensions[i] = extensions[i];
		}
		else if(optionalExtensionsAvailable[i - requiredExtensionCount])
		{
			finalExtensions[optionalIndex] = extensions[i];
			++optionalIndex;
		}
	}

#ifndef NDEBUG
	VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = frMessengerCallback
	};
#endif

	uint32_t vulkanVersion = VK_API_VERSION_1_0;
	#ifdef VK_VERSION_1_1
	const PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion");
	if(vkEnumerateInstanceVersion)
	{
		if(vkEnumerateInstanceVersion(&vulkanVersion) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
	#endif

	const VkApplicationInfo applicationInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = name,
		.applicationVersion = version,
		.pEngineName = "Fraus",
		.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.apiVersion = vulkanVersion
	};

	const VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &applicationInfo,
#ifndef NDEBUG
		.pNext = debugExtensionAvailable ? &messengerCreateInfo : NULL,
		.enabledLayerCount = validationLayersAvailable ? layerCount : 0,
		.ppEnabledLayerNames = layers,
#endif
		.enabledExtensionCount = finalExtensionCount,
		.ppEnabledExtensionNames = finalExtensions
	};

	if(vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	free(finalExtensions);

	if(frLoadInstanceFunctions() != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

#ifndef NDEBUG
	if(debugExtensionAvailable)
	{
		messengerCreateInfo.pUserData = &instance;
		if(vkCreateDebugUtilsMessengerEXT(instance, &messengerCreateInfo, NULL, &messenger) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}
#endif

	return FR_SUCCESS;
}

static FrResult frChoosePhysicalDevice(void)
{
	uint32_t physicalDeviceCount;
	if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	VkPhysicalDevice* const physicalDevices = malloc(physicalDeviceCount * sizeof(physicalDevices[0]));
	if(!physicalDevices)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices) != VK_SUCCESS)
	{
		free(physicalDevices);
		return FR_ERROR_UNKNOWN;
	}

	for(uint32_t i = 1; i < physicalDeviceCount; ++i)
	{
		uint32_t queueFamilyPropertyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyPropertyCount, NULL);
		VkQueueFamilyProperties* const queueFamilyProperties = malloc(queueFamilyPropertyCount * sizeof(queueFamilyProperties[0]));
		if(!queueFamilyProperties)
		{
			free(physicalDevices);
			return FR_ERROR_UNKNOWN;
		}
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyPropertyCount, queueFamilyProperties);

		for(uint32_t j = 0; j < queueFamilyPropertyCount; ++j)
		{
			VkBool32 surfaceSupported;
			if(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], j, surface, &surfaceSupported) != VK_SUCCESS)
			{
				free(queueFamilyProperties);
				free(physicalDevices);
				return FR_ERROR_UNKNOWN;
			}

			if(queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT && surfaceSupported)
			{
				physicalDevice = physicalDevices[i];
				queueFamily = j;

				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(physicalDevice, &properties);

				const VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
				if(counts & VK_SAMPLE_COUNT_64_BIT)
				{
					msaaSamples = VK_SAMPLE_COUNT_64_BIT;
				}
				else if(counts & VK_SAMPLE_COUNT_32_BIT)
				{
					msaaSamples = VK_SAMPLE_COUNT_32_BIT;
				}
				else if(counts & VK_SAMPLE_COUNT_16_BIT)
				{
					msaaSamples = VK_SAMPLE_COUNT_16_BIT;
				}
				else if(counts & VK_SAMPLE_COUNT_8_BIT)
				{
					msaaSamples = VK_SAMPLE_COUNT_8_BIT;
				}
				else if(counts & VK_SAMPLE_COUNT_4_BIT)
				{
					msaaSamples = VK_SAMPLE_COUNT_4_BIT;
				}
				else if(counts & VK_SAMPLE_COUNT_2_BIT)
				{
					msaaSamples = VK_SAMPLE_COUNT_2_BIT;
				}
				else
				{
					msaaSamples = VK_SAMPLE_COUNT_1_BIT;
				}

				free(queueFamilyProperties);
				free(physicalDevices);

				return FR_SUCCESS;
			}
		}

		free(queueFamilyProperties);
	}

	free(physicalDevices);

	return FR_ERROR_UNKNOWN;
}

static FrResult frCreateSurface(void)
{
#ifdef _WIN32
	const VkWin32SurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = windowInstance,
		.hwnd = windowHandle
	};
	if(vkCreateWin32SurfaceKHR(instance, &createInfo, NULL, &surface) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
#else
	const VkXlibSurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
		.dpy = pVulkanData->pApplication->window.pDisplay,
		.window = pVulkanData->pApplication->window.window
	};
	if(vkCreateXlibSurfaceKHR(instance, &createInfo, NULL, &surface) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

static FrResult frCreateDevice(void)
{
	const float priority = 1.f;

	uint32_t availableExtensionCount;
	if(vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &availableExtensionCount, NULL) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	VkExtensionProperties* const availableExtensions = malloc(availableExtensionCount * sizeof(availableExtensions[0]));
	if(!availableExtensions)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &availableExtensionCount, availableExtensions) != VK_SUCCESS)
	{
		free(availableExtensions);
		return FR_ERROR_UNKNOWN;
	}

	const char* extensions[] = {
		// Required
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	const uint32_t extensionCount = FR_LEN(extensions);
	const uint32_t requiredExtensionCount = 1;
	bool* const optionalExtensionsAvailable = NULL;

	const uint32_t finalExtensionCount = frExtensionsAvailable(
		availableExtensions,
		availableExtensionCount,
		extensions,
		extensionCount,
		requiredExtensionCount,
		optionalExtensionsAvailable
	);
	free(availableExtensions);
	if(!finalExtensionCount)
	{
		return FR_ERROR_UNKNOWN;
	}

	const char** finalExtensions = malloc(finalExtensionCount * sizeof(finalExtensions[0]));
	if(!finalExtensions)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	for(uint32_t i = 0, optionalIndex = requiredExtensionCount; i < extensionCount; ++i)
	{
		if(i < requiredExtensionCount)
		{
			finalExtensions[i] = extensions[i];
		}
		else if(optionalExtensionsAvailable[i - requiredExtensionCount])
		{
			finalExtensions[optionalIndex] = extensions[i];
			++optionalIndex;
		}
	}

	const VkDeviceQueueCreateInfo queueCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = queueFamily,
		.queueCount = 1,
		.pQueuePriorities = &priority
	};

	VkPhysicalDeviceFeatures features, wantedFeatures = {VK_FALSE};
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	wantedFeatures.samplerAnisotropy = features.samplerAnisotropy;
	wantedFeatures.sampleRateShading = features.sampleRateShading;

	const VkDeviceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = finalExtensionCount,
		.ppEnabledExtensionNames = finalExtensions,
		.pEnabledFeatures = &wantedFeatures
	};

	if(vkCreateDevice(physicalDevice, &createInfo, NULL, &device) != VK_SUCCESS)
	{
		free(finalExtensions);
		return FR_ERROR_UNKNOWN;
	}

	free(finalExtensions);

	#ifndef NDEBUG
	if(debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_INSTANCE,
			.objectHandle = (uint64_t)instance,
			.pObjectName = "Fraus instance"
		};
		if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}

		nameInfo.objectType = VK_OBJECT_TYPE_SURFACE_KHR;
		nameInfo.objectHandle = (uint64_t)surface;
		nameInfo.pObjectName = "Fraus surface";
		if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}

		nameInfo.objectType = VK_OBJECT_TYPE_PHYSICAL_DEVICE;
		nameInfo.objectHandle = (uint64_t)physicalDevice;
		nameInfo.pObjectName = "Fraus physical device";
		if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}

		nameInfo.objectType = VK_OBJECT_TYPE_DEVICE;
		nameInfo.objectHandle = (uint64_t)device;
		nameInfo.pObjectName = "Fraus device";
		if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}
	#endif

	if(frLoadDeviceFunctions() != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	vkGetDeviceQueue(device, queueFamily, 0, &queue);

	#ifndef NDEBUG
	if(debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_QUEUE,
			.objectHandle = (uint64_t)queue,
			.pObjectName = "Fraus queue"
		};
		if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}
	#endif

	return FR_SUCCESS;
}

static FrResult frCreateSwapchain(void)
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	swapchainExtent = surfaceCapabilities.currentExtent;

	uint32_t surfaceFormatCount;
	if(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, NULL) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	VkSurfaceFormatKHR* const surfaceFormats = malloc(surfaceFormatCount * sizeof(surfaceFormats));
	if(!surfaceFormats)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	if(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats) != VK_SUCCESS)
	{
		free(surfaceFormats);
		return FR_ERROR_UNKNOWN;
	}
	uint32_t surfaceFormatIndex = 0;
	for(uint32_t i = 0; i < surfaceFormatCount; ++i)
	{
		if(surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			surfaceFormatIndex = i;
			break;
		}
	}
	swapchainFormat = surfaceFormats[surfaceFormatIndex].format;

	uint32_t presentModeCount;
	if(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL) != VK_SUCCESS)
	{
		free(surfaceFormats);
		return FR_ERROR_UNKNOWN;
	}
	VkPresentModeKHR* const presentModes = malloc(presentModeCount * sizeof(presentModes));
	if(!presentModes)
	{
		free(surfaceFormats);
		return FR_ERROR_UNKNOWN;
	}
	if(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes) != VK_SUCCESS)
	{
		free(surfaceFormats);
		free(presentModes);
		return FR_ERROR_UNKNOWN;
	}

	uint32_t presentModeIndex;
	uint32_t fifoIndex = presentModeCount;
	for(presentModeIndex = 0; presentModeIndex < presentModeCount; ++presentModeIndex)
	{
		if(presentModes[presentModeIndex] == VK_PRESENT_MODE_FIFO_KHR)
		{
			fifoIndex = presentModeIndex;
			continue;
		}

		if(presentModes[presentModeIndex] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			break;
		}
	}
	if(presentModeIndex == presentModeCount)
	{
		if(fifoIndex < presentModeCount)
		{
			presentModeIndex = fifoIndex;
		}
		else
		{
			presentModeIndex = 0;
		}
	}

	const VkSwapchainCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.minImageCount < surfaceCapabilities.maxImageCount),
		.imageFormat = surfaceFormats[surfaceFormatIndex].format,
		.imageColorSpace = surfaceFormats[surfaceFormatIndex].colorSpace,
		.imageExtent = surfaceCapabilities.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentModes[presentModeIndex],
		.clipped = VK_TRUE,
		.oldSwapchain = swapchain
	};
	if(vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain) != VK_SUCCESS)
	{
		free(surfaceFormats);
		free(presentModes);
		return EXIT_FAILURE;
	}

	#ifndef NDEBUG
	if(debugExtensionAvailable)
	{
		const VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR,
			.objectHandle = (uint64_t)swapchain,
			.pObjectName = "Fraus swapchain"
		};
		if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
		{
			free(surfaceFormats);
			free(presentModes);
			return FR_ERROR_UNKNOWN;
		}
	}
	#endif

	free(surfaceFormats);
	free(presentModes);

	if(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	VkImage* const newImages = realloc(swapchainImages, swapchainImageCount * sizeof(newImages[0]));
	if(!newImages)
	{
		return FR_ERROR_UNKNOWN;
	}
	swapchainImages = newImages;
	if(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages) != VK_SUCCESS)
	{
		free(swapchainImages);
		swapchainImages = NULL;
		return FR_ERROR_UNKNOWN;
	}

	#ifndef NDEBUG
	if(debugExtensionAvailable)
	{
		char imageName[32];

		for(uint32_t i = 0; i < swapchainImageCount; ++i)
		{
			snprintf(imageName, sizeof(imageName), "Fraus swapchain image %"PRIu32, i);

			const VkDebugUtilsObjectNameInfoEXT nameInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_IMAGE,
				.objectHandle = (uint64_t)swapchainImages[i],
				.pObjectName = imageName
			};
			if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
			{
				return FR_ERROR_UNKNOWN;
			}
		}
	}
	#endif

	VkImageView* const newImageViews = realloc(swapchainImageViews, swapchainImageCount * sizeof(newImageViews[0]));
	if(!newImageViews)
	{
		free(swapchainImages);
		swapchainImages = NULL;
		return FR_ERROR_UNKNOWN;
	}
	swapchainImageViews = newImageViews;
	for(uint32_t imageIndex = 0; imageIndex < swapchainImageCount; ++imageIndex)
	{
		if(frCreateImageView(swapchainImages[imageIndex], swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, &swapchainImageViews[imageIndex]) != FR_SUCCESS)
		{
			for(uint32_t j = 0; j < imageIndex; ++j)
			{
				vkDestroyImageView(device, swapchainImageViews[j], NULL);
			}
			free(swapchainImageViews);
			free(swapchainImages);
			swapchainImageViews = NULL;
			swapchainImages = NULL;
			return FR_ERROR_UNKNOWN;
		}
	}

	#ifndef NDEBUG
	if(debugExtensionAvailable)
	{
		char imageViewName[32];

		for(uint32_t i = 0; i < swapchainImageCount; ++i)
		{
			snprintf(imageViewName, sizeof(imageViewName), "Fraus swapchain image view %"PRIu32, i);

			const VkDebugUtilsObjectNameInfoEXT nameInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
				.objectHandle = (uint64_t)swapchainImageViews[i],
				.pObjectName = imageViewName
			};
			if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
			{
				return FR_ERROR_UNKNOWN;
			}
		}
	}
	#endif

	return FR_SUCCESS;
}

static FrResult frCreateRenderPass(void)
{
	const VkAttachmentDescription attachments[] = {
		{
			.format = swapchainFormat,
			.samples = msaaSamples,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		},
		{
			.format = VK_FORMAT_D24_UNORM_S8_UINT,
			.samples = msaaSamples,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		},
		{
			.format = swapchainFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		}
	};

	const VkAttachmentReference colorAttachmentReference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	const VkAttachmentReference depthAttachmentReference = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
	const VkAttachmentReference resolveAttachmentReference = {
		.attachment = 2,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	const VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentReference,
		.pResolveAttachments = &resolveAttachmentReference,
		.pDepthStencilAttachment = &depthAttachmentReference
	};

	const VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		.dstAccessMask =  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	};

	const VkRenderPassCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = FR_LEN(attachments),
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	if(vkCreateRenderPass(device, &createInfo, NULL, &renderPass) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	#ifndef NDEBUG
	if(debugExtensionAvailable)
	{
		const VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_RENDER_PASS,
			.objectHandle = (uint64_t)renderPass,
			.pObjectName = "Fraus render pass"
		};
		if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}
	#endif

	return FR_SUCCESS;
}

static FrResult frCreateFramebuffers(void)
{
	VkFramebuffer* const newFramebuffers = realloc(framebuffers, swapchainImageCount * sizeof(newFramebuffers[0]));
	if(!newFramebuffers)
	{
		free(framebuffers);
		return FR_ERROR_UNKNOWN;
	}
	framebuffers = newFramebuffers;

	for(uint32_t imageIndex = 0; imageIndex < swapchainImageCount; ++imageIndex)
	{
		const VkImageView attachments[] = {
			colorImageView,
			depthImageView,
			swapchainImageViews[imageIndex]
		};
		const VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass,
			.attachmentCount = FR_LEN(attachments),
			.pAttachments = attachments,
			.width = swapchainExtent.width,
			.height = swapchainExtent.height,
			.layers = 1
		};
		if(vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffers[imageIndex]) != VK_SUCCESS)
		{
			for(uint32_t j = 0; j < imageIndex; ++j)
			{
				vkDestroyFramebuffer(device, framebuffers[j], NULL);
			}

			free(framebuffers);
			framebuffers = NULL;
			return FR_ERROR_UNKNOWN;
		}
	}

	#ifndef NDEBUG
	if(debugExtensionAvailable)
	{
		char framebufferName[32];

		for(uint32_t i = 0; i < swapchainImageCount; ++i)
		{
			snprintf(framebufferName, sizeof(framebufferName), "Fraus framebuffer %"PRIu32, i);

			const VkDebugUtilsObjectNameInfoEXT nameInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_FRAMEBUFFER,
				.objectHandle = (uint64_t)framebuffers[i],
				.pObjectName = framebufferName
			};
			if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
			{
				return FR_ERROR_UNKNOWN;
			}
		}
	}
	#endif

	return FR_SUCCESS;
}

static FrResult frCreateShaderModule(const char* path, VkShaderModule* pShaderModule, FrShaderInfo* pInfo)
{
	// Prepare create info
	VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO
	};

	// Open file
	FILE* const file = fopen(path, "rb");
	if(!file)
	{
		return FR_ERROR_FILE_NOT_FOUND;
	}
	
	// Get file size
	fseek(file, 0, SEEK_END);
	createInfo.codeSize = ftell(file);
	if(createInfo.codeSize == (size_t)-1L)
	{
		fclose(file);
		return FR_ERROR_UNKNOWN;
	}

	// SPIR-V code must contain 32-bit words
	if(createInfo.codeSize % sizeof(createInfo.pCode[0]) != 0)
	{
		fclose(file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Allocate memory for SPIR-V code
	uint32_t* const code = malloc(createInfo.codeSize);
	if(!code)
	{
		fclose(file);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	// Read SPIR-V code
	fseek(file, 0, SEEK_SET);
	if(fread(code, sizeof(code[0]), createInfo.codeSize / sizeof(code[0]), file) != createInfo.codeSize / sizeof(code[0]))
	{
		free(code);
		fclose(file);
		return FR_ERROR_UNKNOWN;
	}
	createInfo.pCode = code;

	// Close file
	fclose(file);

	if(code[0] != SpvMagicNumber)
	{
		for(uint32_t i = 0; i < createInfo.codeSize / sizeof(code[0]); ++i)
		{
			code[i] = FR_SWAP_BYTES_U32(code[i]);
		}

		if(code[0] != SpvMagicNumber)
		{
			free(code);
			return FR_ERROR_CORRUPTED_FILE;
		}
	}
	
	// Create shader module
	if(vkCreateShaderModule(device, &createInfo, NULL, pShaderModule) != VK_SUCCESS)
	{
		free(code);
		return FR_ERROR_UNKNOWN;
	}

	if(frParseSpirv(code, createInfo.codeSize / sizeof(code[0]), pInfo) != FR_SUCCESS)
	{
		free(code);
		return FR_ERROR_UNKNOWN;
	}

	// Free SPIR-V code
	free(code);

	return FR_SUCCESS;
}

static int frComparePushConstants(const void* pAV, const void* pBV)
{
	const VkPushConstantRange* pA = pAV;
	const VkPushConstantRange* pB = pBV;

	if(pA->offset < pB->offset)
	{
		return -1;
	}
	if(pA->offset > pB->offset)
	{
		return 1;
	}
	return 0;
}

FrResult frCreateGraphicsPipeline(const FrPipelineCreateInfo* pCreateInfo)
{
	if(!pCreateInfo || !pCreateInfo->vertexShaderPath || !pCreateInfo->fragmentShaderPath || (pCreateInfo->vertexInputRateCount && (!pCreateInfo->vertexInputRates || !pCreateInfo->vertexInputStrides)))
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

	if(frPushBackPipelineVector(&graphicsPipelines, (FrPipeline){0}) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Shader stages
	FrShaderInfo vertexInfo;
	VkShaderModule vertexModule;
	if(frCreateShaderModule(pCreateInfo->vertexShaderPath, &vertexModule, &vertexInfo) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	for(uint32_t i = 0; i < vertexInfo.bindingCount; ++i)
	{
		vertexInfo.bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	}
	for(uint32_t i = 0; i < vertexInfo.pushConstantCount; ++i)
	{
		vertexInfo.pushConstants[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	}

	FrShaderInfo fragmentInfo;
	VkShaderModule fragmentModule;
	if(frCreateShaderModule(pCreateInfo->fragmentShaderPath, &fragmentModule, &fragmentInfo) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	for(uint32_t i = 0; i < fragmentInfo.bindingCount; ++i)
	{
		fragmentInfo.bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	for(uint32_t i = 0; i < fragmentInfo.pushConstantCount; ++i)
	{
		fragmentInfo.pushConstants[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	}

	if(vertexInfo.outputCount != fragmentInfo.inputCount)
	{
		free(vertexInfo.inputs);
		free(fragmentInfo.inputs);
		free(vertexInfo.outputs);
		free(fragmentInfo.outputs);
		free(vertexInfo.bindings);
		free(fragmentInfo.bindings);
		free(vertexInfo.pushConstants);
		free(fragmentInfo.pushConstants);
		return FR_ERROR_UNKNOWN;
	}
	for(uint32_t i = 0; i < vertexInfo.outputCount; ++i)
	{
		if(vertexInfo.outputs[i].location != fragmentInfo.inputs[i].location || vertexInfo.outputs[i].size != fragmentInfo.inputs[i].size)
		{
			free(vertexInfo.inputs);
			free(fragmentInfo.inputs);
			free(vertexInfo.outputs);
			free(fragmentInfo.outputs);
			free(vertexInfo.bindings);
			free(fragmentInfo.bindings);
			free(vertexInfo.pushConstants);
			free(fragmentInfo.pushConstants);
			return FR_ERROR_UNKNOWN;
		}
	}

	const uint32_t bindingCount = vertexInfo.bindingCount + fragmentInfo.bindingCount;
	VkDescriptorSetLayoutBinding* const bindings = malloc(bindingCount * sizeof(bindings[0]));
	if(!bindings)
	{
		free(vertexInfo.bindings);
		free(fragmentInfo.bindings);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	frMergeSorted(vertexInfo.bindingCount, vertexInfo.bindings, fragmentInfo.bindingCount, fragmentInfo.bindings, bindings, sizeof(bindings[0]), frCompareBindings);

	const uint32_t pushConstantCount = vertexInfo.pushConstantCount + fragmentInfo.pushConstantCount;
	graphicsPipelines.data[graphicsPipelines.size - 1].hasPushConstants = pushConstantCount > 0;
	VkPushConstantRange* pushConstants = NULL;
	if(pushConstantCount)
	{
		pushConstants = malloc(pushConstantCount * sizeof(pushConstants[0]));
		if(!pushConstants)
		{
			free(bindings);
			free(vertexInfo.bindings);
			free(fragmentInfo.bindings);
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}
		frMergeSorted(vertexInfo.pushConstantCount, vertexInfo.pushConstants, fragmentInfo.pushConstantCount, fragmentInfo.pushConstants, pushConstants, sizeof(pushConstants[0]), frComparePushConstants);
	}

	free(fragmentInfo.inputs);
	free(vertexInfo.outputs);
	free(fragmentInfo.outputs);
	free(vertexInfo.bindings);
	free(fragmentInfo.bindings);
	free(vertexInfo.pushConstants);
	free(fragmentInfo.pushConstants);

	#ifndef NDEBUG
	if(debugExtensionAvailable)
	{
		char name[32];

		snprintf(name, sizeof(name), "Fraus vertex shader module %zu", graphicsPipelines.size - 1);
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_SHADER_MODULE,
			.objectHandle = (uint64_t)vertexModule,
			.pObjectName = name
		};
		if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}

		snprintf(name, sizeof(name), "Fraus fragment shader module %zu", graphicsPipelines.size - 1);
		nameInfo.objectHandle = (uint64_t)fragmentModule;
		nameInfo.pObjectName = name;
		if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}
	#endif

	const VkPipelineShaderStageCreateInfo stageInfos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertexModule,
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragmentModule,
			.pName = "main",
		}
	};

	// Vertex input
	VkVertexInputAttributeDescription* const attributes = malloc(vertexInfo.inputCount * sizeof(attributes[0]));
	if(!attributes)
	{
		free(bindings);
		free(pushConstants);
		free(vertexInfo.inputs);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	uint32_t offset = 0;
	uint32_t total = 0;
	uint32_t bindingI = 0;
	for(uint32_t i = 0; i < vertexInfo.inputCount; ++i)
	{
		attributes[i].binding = bindingI;
		attributes[i].location = vertexInfo.inputs[i].location;
		attributes[i].offset = offset;
		attributes[i].format = vertexInfo.inputs[i].format;

		offset += vertexInfo.inputs[i].size;
		total += vertexInfo.inputs[i].size;

		if(pCreateInfo->vertexInputRateCount && pCreateInfo->vertexInputStrides[bindingI] < offset)
		{
			return FR_ERROR_INVALID_ARGUMENT;
		}
		if(pCreateInfo->vertexInputRateCount && pCreateInfo->vertexInputStrides[bindingI] == offset)
		{
			++bindingI;
			offset = 0;
		}
	}
	free(vertexInfo.inputs);

	VkVertexInputBindingDescription binding = {
		.binding = 0,
		.stride = offset,
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};
	uint32_t inputBindingCount;
	VkVertexInputBindingDescription* inputBindings;
	if(pCreateInfo->vertexInputRateCount)
	{
		inputBindingCount = pCreateInfo->vertexInputRateCount;
		inputBindings = malloc(inputBindingCount * sizeof(inputBindings[0]));
		if(!inputBindings)
		{
			free(bindings);
			free(attributes);
			free(pushConstants);
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}

		uint32_t s = 0;
		for(uint32_t i = 0; i < inputBindingCount; s += pCreateInfo->vertexInputStrides[i], ++i)
		{
			inputBindings[i].binding = i;
			inputBindings[i].stride = pCreateInfo->vertexInputStrides[i];
			inputBindings[i].inputRate = pCreateInfo->vertexInputRates[i];
		}

		if(total != s)
		{
			free(bindings);
			free(attributes);
			free(pushConstants);
			free(inputBindings);
			return FR_ERROR_INVALID_ARGUMENT;
		}
	}
	else
	{
		inputBindingCount = 1;
		inputBindings = &binding;
	}

	const VkPipelineVertexInputStateCreateInfo vertexInput = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = inputBindingCount,
		.pVertexBindingDescriptions = inputBindings,
		.vertexAttributeDescriptionCount = vertexInfo.inputCount,
		.pVertexAttributeDescriptions = attributes
	};

	// Input assembly
	const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	// Viewport
	const VkPipelineViewportStateCreateInfo viewport = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1
	};

	// Rasterization
	const VkPipelineRasterizationStateCreateInfo rasterization = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.f
	};

	// Multisample
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	const VkPipelineMultisampleStateCreateInfo multisample = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = msaaSamples,
		.sampleShadingEnable = features.sampleRateShading,
		.minSampleShading = 1.f
	};

	// Depth stencil
	const VkPipelineDepthStencilStateCreateInfo depthStencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = pCreateInfo->depthTestDisable ? VK_FALSE : VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE
	};

	// Color blend
	const VkPipelineColorBlendAttachmentState colorAttachment = {
		.blendEnable = pCreateInfo->alphaBlendEnable ? VK_TRUE : VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	const VkPipelineColorBlendStateCreateInfo colorBlend = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment
	};

	// Dynamic
	const VkDynamicState dynamics[] = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};

	VkPipelineDynamicStateCreateInfo dynamic = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = FR_LEN(dynamics),
		.pDynamicStates = dynamics
	};

	graphicsPipelines.data[graphicsPipelines.size - 1].descriptorTypeCount = bindingCount;
	graphicsPipelines.data[graphicsPipelines.size - 1].descriptorTypes = malloc(bindingCount * sizeof(graphicsPipelines.data[graphicsPipelines.size - 1].descriptorTypes[0]));
	if(!graphicsPipelines.data[graphicsPipelines.size - 1].descriptorTypes)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	for(uint32_t i = 0; i < bindingCount; ++i)
	{
		graphicsPipelines.data[graphicsPipelines.size - 1].descriptorTypes[i] = bindings[i].descriptorType;
	}

	const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = bindingCount,
		.pBindings = bindings
	};
	if(vkCreateDescriptorSetLayout(
		device,
		&descriptorSetLayoutInfo,
		NULL,
		&graphicsPipelines.data[graphicsPipelines.size - 1].descriptorSetLayout
	) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	free(bindings);

	// Layout
	const VkPipelineLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &graphicsPipelines.data[graphicsPipelines.size - 1].descriptorSetLayout,
		.pushConstantRangeCount = pushConstantCount,
		.pPushConstantRanges = pushConstants
	};

	if(vkCreatePipelineLayout(
		device,
		&layoutInfo,
		NULL,
		&graphicsPipelines.data[graphicsPipelines.size - 1].pipelineLayout
	) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	free(pushConstants);

	// Pipeline
	const VkGraphicsPipelineCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = FR_LEN(stageInfos),
		.pStages = stageInfos,
		.pVertexInputState = &vertexInput,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewport,
		.pRasterizationState = &rasterization,
		.pMultisampleState = &multisample,
		.pDepthStencilState = &depthStencil,
		.pColorBlendState = &colorBlend,
		.pDynamicState = &dynamic,
		.layout = graphicsPipelines.data[graphicsPipelines.size - 1].pipelineLayout,
		.renderPass = renderPass,
		.subpass = 0
	};

	if(vkCreateGraphicsPipelines(
		device,
		VK_NULL_HANDLE,
		1,
		&createInfo,
		NULL,
		&graphicsPipelines.data[graphicsPipelines.size - 1].pipeline
	) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	free(attributes);

	#ifndef NDEBUG
	if(debugExtensionAvailable)
	{
		char pipelineName[32];
		snprintf(pipelineName, sizeof(pipelineName), "Fraus graphics pipeline %zu", graphicsPipelines.size - 1);

		const VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_PIPELINE,
			.objectHandle = (uint64_t)graphicsPipelines.data[graphicsPipelines.size - 1].pipeline,
			.pObjectName = pipelineName
		};
		if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}
	#endif

	vkDestroyShaderModule(device, vertexModule, NULL);
	vkDestroyShaderModule(device, fragmentModule, NULL);

	return FR_SUCCESS;
}

FrResult frCreateUniformBuffer(VkDeviceSize size)
{
	if(frPushBackUniformBufferVector(&uniformBuffers, (FrUniformBuffer){0}) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	uniformBuffers.data[uniformBuffers.size - 1].buffersSize = size;

	// Create uniform buffers
	for(uint32_t frameIndex = 0; frameIndex < FR_FRAMES_IN_FLIGHT; ++frameIndex)
	{
		if(frCreateBuffer(
			size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers.data[uniformBuffers.size - 1].buffers[frameIndex],
			&uniformBuffers.data[uniformBuffers.size - 1].bufferMemories[frameIndex]
		) != FR_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}

		if(vkMapMemory(device, uniformBuffers.data[uniformBuffers.size - 1].bufferMemories[frameIndex], 0, size, 0, &uniformBuffers.data[uniformBuffers.size - 1].bufferDatas[frameIndex]) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}

	return FR_SUCCESS;
}

FrResult frCreateStorageBuffer(VkDeviceSize size)
{
	if(frPushBackStorageBufferVector(&storageBuffers, (FrStorageBuffer){0}) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	storageBuffers.data[storageBuffers.size - 1].bufferSize = size;

	// Create storage buffer
	if(frCreateBuffer(
		size,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&storageBuffers.data[storageBuffers.size - 1].buffer,
		&storageBuffers.data[storageBuffers.size - 1].bufferMemory
	) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

FrResult frSetStorageBufferData(uint32_t storageBufferIndex, const void* data, VkDeviceSize size)
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	if(frCreateBuffer(
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		&stagingBufferMemory
	) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	void* mappedData;
	if(vkMapMemory(device, stagingBufferMemory, 0, size, 0, &mappedData) != VK_SUCCESS)
	{
		vkDestroyBuffer(device, stagingBuffer, NULL);
		vkFreeMemory(device, stagingBufferMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}
	memcpy(mappedData, data, size);
	vkUnmapMemory(device, stagingBufferMemory);

	if(frCopyBuffer(stagingBuffer, storageBuffers.data[storageBufferIndex].buffer, size) != FR_SUCCESS)
	{
		vkDestroyBuffer(device, stagingBuffer, NULL);
		vkFreeMemory(device, stagingBufferMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);

	return FR_SUCCESS;
}

static FrResult frCreateSampler(void)
{
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	const VkSamplerCreateInfo samplerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.f,
		.anisotropyEnable = features.samplerAnisotropy,
		.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.f,
		.maxLod = VK_LOD_CLAMP_NONE,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};
	if(vkCreateSampler(device, &samplerCreateInfo, NULL, &textureSampler) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

static FrResult frCreateColorImage(void)
{
	if(frCreateImage(swapchainExtent.width, swapchainExtent.height, 1, msaaSamples, swapchainFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &colorImage, &colorImageMemory) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(frCreateImageView(colorImage, swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, &colorImageView) != FR_SUCCESS)
	{
		vkDestroyImage(device, colorImage, NULL);
		vkFreeMemory(device, colorImageMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

static FrResult frCreateDepthImage(void)
{
	VkFormatProperties properties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, VK_FORMAT_D24_UNORM_S8_UINT, &properties);
	if(!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
	{
		return FR_ERROR_UNKNOWN;
	}

	if(frCreateImage(
		swapchainExtent.width,
		swapchainExtent.height,
		1,
		msaaSamples,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&depthImage,
		&depthImageMemory
	) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	if(frCreateImageView(depthImage, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, 1, &depthImageView) != FR_SUCCESS)
	{
		vkDestroyImage(device, depthImage, NULL);
		vkFreeMemory(device, depthImageMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

static FrResult frCreateCommandPools(void)
{
	frameInFlightIndex = 0;

	for(uint32_t i = 0; i < FR_FRAMES_IN_FLIGHT; ++i)
	{
		const VkCommandPoolCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = queueFamily
		};
		if(vkCreateCommandPool(device, &createInfo, NULL, &commandPools[i]) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}

		#ifndef NDEBUG
		if(debugExtensionAvailable)
		{
			char commandPoolName[32];
			snprintf(commandPoolName, sizeof(commandPoolName), "Fraus command pool %"PRIu32, i);

			const VkDebugUtilsObjectNameInfoEXT nameInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_COMMAND_POOL,
				.objectHandle = (uint64_t)commandPools[i],
				.pObjectName = commandPoolName
			};
			if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
			{
				return FR_ERROR_UNKNOWN;
			}
		}
		#endif

		const VkCommandBufferAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = commandPools[i],
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};
		if(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffers[i]) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}

		#ifndef NDEBUG
		if(debugExtensionAvailable)
		{
			char commandBufferName[32];
			snprintf(commandBufferName, sizeof(commandBufferName), "Fraus command buffer %"PRIu32, i);

			const VkDebugUtilsObjectNameInfoEXT nameInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
				.objectHandle = (uint64_t)commandBuffers[i],
				.pObjectName = commandBufferName
			};
			if(vkSetDebugUtilsObjectNameEXT(device, &nameInfo) != VK_SUCCESS)
			{
				return FR_ERROR_UNKNOWN;
			}
		}
		#endif

		const VkSemaphoreCreateInfo semaphoreCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		const VkFenceCreateInfo fenceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};
		if(vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &imageAvailableSemaphores[i]) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
		if(vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &renderFinishedSemaphores[i]) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
		if(vkCreateFence(device, &fenceCreateInfo, NULL, &frameInFlightFences[i]) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}

	return FR_SUCCESS;
}

FrResult frDrawFrame(void)
{
	// Wait for fence
	if(vkWaitForFences(device, 1, &frameInFlightFences[frameInFlightIndex], VK_TRUE, UINT64_MAX) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(vkResetFences(device, 1, &frameInFlightFences[frameInFlightIndex]) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Acquire image
	VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[frameInFlightIndex], VK_NULL_HANDLE, &swapchainImageIndex);
	if(result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		windowResized = true;
		return FR_SUCCESS;
	}
	else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Reset command pool
	if(vkResetCommandPool(device, commandPools[frameInFlightIndex], 0) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Begin command buffer and render pass
	const VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	if(vkBeginCommandBuffer(commandBuffers[frameInFlightIndex], &beginInfo) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	const VkClearValue clearColor = {
		.color.float32 = {0.f, 0.f, 0.f, 0.f}
	};
	const VkClearValue clearDepthStencil = {
		.depthStencil = {1.f, 0}
	};
	const VkClearValue clearValues[] = {
		clearColor,
		clearDepthStencil
	};
	const VkRenderPassBeginInfo renderPassBegin = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = framebuffers[swapchainImageIndex],
		.renderArea.offset = {0, 0},
		.renderArea.extent = swapchainExtent,
		.clearValueCount = FR_LEN(clearValues),
		.pClearValues = clearValues
	};
	vkCmdBeginRenderPass(commandBuffers[frameInFlightIndex], &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

	// Update camera
	float cameraMatrix[16];
	frGetCameraMatrix(cameraMatrix);
	memcpy(uniformBuffers.data[0].bufferDatas[frameInFlightIndex], cameraMatrix, sizeof(cameraMatrix));

	const VkViewport viewport = {
		.x = 0.f,
		.y = 0.f,
		.width = (float)swapchainExtent.width,
		.height = (float)swapchainExtent.height,
		.minDepth = 0.f,
		.maxDepth = 1.f
	};
	vkCmdSetViewport(commandBuffers[frameInFlightIndex], 0, 1, &viewport);

	const VkRect2D scissor = {
		.offset = {0, 0},
		.extent = swapchainExtent
	};
	vkCmdSetScissor(commandBuffers[frameInFlightIndex], 0, 1, &scissor);

	const VkDeviceSize offsets[] = {0};
	static uint32_t lastPipelineIndex = UINT32_MAX;
	const uint32_t ordering[] = {2, 0, 1, 3, 4};
	for(uint32_t objectIndex = 0; objectIndex < frObjects.size - 1; ++objectIndex)
	{
		const FrVulkanObject* const pObject = &frObjects.data[ordering[objectIndex]];

		if(pObject->pipelineIndex != lastPipelineIndex)
		{
			vkCmdBindPipeline(commandBuffers[frameInFlightIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines.data[pObject->pipelineIndex].pipeline);
			lastPipelineIndex = pObject->pipelineIndex;
		}

		vkCmdBindDescriptorSets(commandBuffers[frameInFlightIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines.data[pObject->pipelineIndex].pipelineLayout, 0, 1, &pObject->descriptorSets[frameInFlightIndex], 0, NULL);
		if(graphicsPipelines.data[pObject->pipelineIndex].hasPushConstants)
		{
			vkCmdPushConstants(commandBuffers[frameInFlightIndex], graphicsPipelines.data[pObject->pipelineIndex].pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pObject->transformation), pObject->transformation);
		}

		vkCmdBindVertexBuffers(commandBuffers[frameInFlightIndex], 0, 1, &pObject->buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffers[frameInFlightIndex], pObject->buffer, pObject->vertexCount * sizeof(pObject->vertices[0]), VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffers[frameInFlightIndex], pObject->indexCount, 1, 0, 0, 0);
	}

	// Draw text
	vkCmdBindPipeline(commandBuffers[frameInFlightIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines.data[2].pipeline);
	vkCmdBindDescriptorSets(commandBuffers[frameInFlightIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines.data[2].pipelineLayout, 0, 1, &frObjects.data[5].descriptorSets[frameInFlightIndex], 0, NULL);
	const VkDeviceSize offsetss[] = {0, 4 * 8 + 2 * 6};
	const VkBuffer buffers[] = {instanceBuffer, instanceBuffer};
	vkCmdBindVertexBuffers(commandBuffers[frameInFlightIndex], 0, 2, buffers, offsetss);
	vkCmdBindIndexBuffer(commandBuffers[frameInFlightIndex], instanceBuffer, 4 * 8, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(commandBuffers[frameInFlightIndex], 6, instanceCount, 0, 0, 0);

	// End render pass and command buffer
	vkCmdEndRenderPass(commandBuffers[frameInFlightIndex]);

	if(vkEndCommandBuffer(commandBuffers[frameInFlightIndex]) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	const VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	const VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &imageAvailableSemaphores[frameInFlightIndex],
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffers[frameInFlightIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &renderFinishedSemaphores[frameInFlightIndex]
	};
	if(vkQueueSubmit(queue, 1, &submitInfo, frameInFlightFences[frameInFlightIndex]) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	VkResult presentResult;
	const VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &renderFinishedSemaphores[frameInFlightIndex],
		.swapchainCount = 1,
		.pSwapchains = &swapchain,
		.pImageIndices = &swapchainImageIndex,
		.pResults = &presentResult
	};
	result = vkQueuePresentKHR(queue, &presentInfo);
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		windowResized = true;
	}
	else if(result != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	frameInFlightIndex = (frameInFlightIndex + 1) % FR_FRAMES_IN_FLIGHT;

	return FR_SUCCESS;
}

FrResult frRecreateSwapchain(void)
{
	if(vkDeviceWaitIdle(device) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	const VkSwapchainKHR oldSwapchain = swapchain;

	for(uint32_t imageIndex = 0; imageIndex < swapchainImageCount; ++imageIndex)
	{
		vkDestroyFramebuffer(device, framebuffers[imageIndex], NULL);
		vkDestroyImageView(device, swapchainImageViews[imageIndex], NULL);
	}
	vkDestroyImageView(device, depthImageView, NULL);
	vkDestroyImage(device, depthImage, NULL);
	vkFreeMemory(device, depthImageMemory, NULL);
	vkDestroyImageView(device, colorImageView, NULL);
	vkDestroyImage(device, colorImage, NULL);
	vkFreeMemory(device, colorImageMemory, NULL);

	if(frCreateSwapchain() != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(frCreateColorImage() != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(frCreateDepthImage() != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(frCreateFramebuffers() != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	vkDestroySwapchainKHR(device, oldSwapchain, NULL);

	windowResized = false;

	return FR_SUCCESS;
}
