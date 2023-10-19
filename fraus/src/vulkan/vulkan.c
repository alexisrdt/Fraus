#include "../../include/fraus/vulkan/vulkan.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../include/fraus/fraus.h"

FR_DEFINE_VECTOR(FrPipeline, Pipeline)
FR_DEFINE_VECTOR(FrUniformBuffer, UniformBuffer)
FR_DEFINE_VECTOR(FrTexture, Texture)

FrResult frCreateVulkanData(FrVulkanData* pVulkanData, const char* pName, uint32_t version)
{
	pVulkanData->swapchain = VK_NULL_HANDLE;
	pVulkanData->pImages = NULL;
	pVulkanData->pImageViews = NULL;
	pVulkanData->pFramebuffers = NULL;

	if(frCreateVulkanObjectVector(&pVulkanData->objects) != FR_SUCCESS) return EXIT_FAILURE;
	if(frCreatePipelineVector(&pVulkanData->graphicsPipelines) != FR_SUCCESS) return EXIT_FAILURE;
	if(frCreateUniformBufferVector(&pVulkanData->uniformBuffers) != FR_SUCCESS) return EXIT_FAILURE;
	if(frCreateTextureVector(&pVulkanData->textures) != FR_SUCCESS) return EXIT_FAILURE;

	if(frCreateInstance(pVulkanData, pName, version) != FR_SUCCESS) return EXIT_FAILURE;
	if(frCreateSurface(pVulkanData) != FR_SUCCESS) return EXIT_FAILURE;
	if(frChoosePhysicalDevice(pVulkanData) != FR_SUCCESS) return EXIT_FAILURE;
	if(frCreateDevice(pVulkanData) != FR_SUCCESS) return EXIT_FAILURE;
	if(frCreateSwapchain(pVulkanData) != FR_SUCCESS) return EXIT_FAILURE;
	if(frCreateRenderPass(pVulkanData) != FR_SUCCESS) return EXIT_FAILURE;
	if(frCreateColorImage(pVulkanData) != FR_SUCCESS) return EXIT_FAILURE;
	if(frCreateDepthImage(pVulkanData) != FR_SUCCESS) return EXIT_FAILURE;
	if(frCreateFramebuffers(pVulkanData) != FR_SUCCESS) return EXIT_FAILURE;
	if(frCreateCommandPools(pVulkanData) != FR_SUCCESS) return EXIT_FAILURE;
	if(frCreateSampler(pVulkanData) != FR_SUCCESS) return EXIT_FAILURE;
	if(frCreateUniformBuffer(pVulkanData, sizeof(float[16])) != FR_SUCCESS) return EXIT_FAILURE; // Camera transform buffer

	return FR_SUCCESS;
}

FrResult frDestroyVulkanData(FrVulkanData* pVulkanData)
{
	free(pVulkanData->pThreads);
	free(pVulkanData->pThreadsData);
	frDestroyConditionVariable(&pVulkanData->frameEndConditionVariable);
	frDestroyMutex(&pVulkanData->frameEndMutex);
	frDestroyConditionVariable(&pVulkanData->frameBeginConditionVariable);
	frDestroyMutex(&pVulkanData->frameBeginMutex) ;

	for(uint32_t i = 0; i < pVulkanData->threadCount * FR_FRAMES_IN_FLIGHT; ++i)
	{
		pVulkanData->functions.vkDestroyCommandPool(pVulkanData->device, pVulkanData->pCommandPools[i], NULL);
	}
	free(pVulkanData->pCommandPools);
	free(pVulkanData->pSecondaryCommandBuffers);
	for(uint32_t i = 0; i < FR_FRAMES_IN_FLIGHT; ++i)
	{
		pVulkanData->functions.vkDestroySemaphore(pVulkanData->device, pVulkanData->imageAvailableSemaphores[i], NULL);
		pVulkanData->functions.vkDestroySemaphore(pVulkanData->device, pVulkanData->renderFinishedSemaphores[i], NULL);
		pVulkanData->functions.vkDestroyFence(pVulkanData->device, pVulkanData->frameInFlightFences[i], NULL);
	}
	pVulkanData->functions.vkDestroySampler(pVulkanData->device, pVulkanData->textureSampler, NULL);
	
	for(uint32_t textureIndex = 0; textureIndex < pVulkanData->textures.size; ++textureIndex)
	{
		pVulkanData->functions.vkDestroyImageView(pVulkanData->device, pVulkanData->textures.pData[textureIndex].imageView, NULL);
		pVulkanData->functions.vkDestroyImage(pVulkanData->device, pVulkanData->textures.pData[textureIndex].image, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, pVulkanData->textures.pData[textureIndex].imageMemory, NULL);
	}
	frDestroyTextureVector(&pVulkanData->textures);

	for(uint32_t uniformBufferIndex = 0; uniformBufferIndex < pVulkanData->uniformBuffers.size; ++uniformBufferIndex)
	{
		for(uint32_t frameIndex = 0; frameIndex < FR_FRAMES_IN_FLIGHT; ++frameIndex)
		{
			pVulkanData->functions.vkDestroyBuffer(pVulkanData->device, pVulkanData->uniformBuffers.pData[uniformBufferIndex].buffers[frameIndex], NULL);
			pVulkanData->functions.vkFreeMemory(pVulkanData->device, pVulkanData->uniformBuffers.pData[uniformBufferIndex].bufferMemories[frameIndex], NULL);
		}
	}
	frDestroyUniformBufferVector(&pVulkanData->uniformBuffers);

	for(uint32_t objectIndex = 0; objectIndex < pVulkanData->objects.size; ++objectIndex)
	{
		frDestroyObject(pVulkanData, &pVulkanData->objects.pData[objectIndex]);
	}
	frDestroyVulkanObjectVector(&pVulkanData->objects);

	for(uint32_t pipelineIndex = 0; pipelineIndex < pVulkanData->graphicsPipelines.size; ++pipelineIndex)
	{
		pVulkanData->functions.vkDestroyPipeline(pVulkanData->device, pVulkanData->graphicsPipelines.pData[pipelineIndex].pipeline, NULL);
		pVulkanData->functions.vkDestroyPipelineLayout(pVulkanData->device, pVulkanData->graphicsPipelines.pData[pipelineIndex].pipelineLayout, NULL);
		pVulkanData->functions.vkDestroyDescriptorSetLayout(pVulkanData->device, pVulkanData->graphicsPipelines.pData[pipelineIndex].descriptorSetLayout, NULL);
		free(pVulkanData->graphicsPipelines.pData[pipelineIndex].pDescriptorTypes);
	}
	frDestroyPipelineVector(&pVulkanData->graphicsPipelines);

	pVulkanData->functions.vkDestroyRenderPass(pVulkanData->device, pVulkanData->renderPass, NULL);
	pVulkanData->functions.vkDestroyImageView(pVulkanData->device, pVulkanData->depthImageView, NULL);
	pVulkanData->functions.vkDestroyImage(pVulkanData->device, pVulkanData->depthImage, NULL);
	pVulkanData->functions.vkFreeMemory(pVulkanData->device, pVulkanData->depthImageMemory, NULL);
	pVulkanData->functions.vkDestroyImageView(pVulkanData->device, pVulkanData->colorImageView, NULL);
	pVulkanData->functions.vkDestroyImage(pVulkanData->device, pVulkanData->colorImage, NULL);
	pVulkanData->functions.vkFreeMemory(pVulkanData->device, pVulkanData->colorImageMemory, NULL);
	for(uint32_t i = 0; i < pVulkanData->imageCount; ++i)
	{
		pVulkanData->functions.vkDestroyFramebuffer(pVulkanData->device, pVulkanData->pFramebuffers[i], NULL);
		pVulkanData->functions.vkDestroyImageView(pVulkanData->device, pVulkanData->pImageViews[i], NULL);
	}
	free(pVulkanData->pFramebuffers);
	free(pVulkanData->pImageViews);
	free(pVulkanData->pImages);
	pVulkanData->functions.vkDestroySwapchainKHR(pVulkanData->device, pVulkanData->swapchain, NULL);
	pVulkanData->functions.vkDestroyDevice(pVulkanData->device, NULL);
	pVulkanData->functions.vkDestroySurfaceKHR(pVulkanData->instance, pVulkanData->surface, NULL);
#ifndef NDEBUG
	if (pVulkanData->debugExtensionAvailable)
	{
		pVulkanData->functions.vkDestroyDebugUtilsMessengerEXT(pVulkanData->instance, pVulkanData->messenger, NULL);
	}
#endif
	pVulkanData->functions.vkDestroyInstance(pVulkanData->instance, NULL);

	return FR_SUCCESS;
}

#ifndef NDEBUG
/*
 * Check that required layers are available
 * - pAvailableLayers: available layers
 * - availableLayerCount: number of available layers
 * - ppLayers: required layers
 * - layerCount: number of required layers
 */
static bool frLayersAvailable(VkLayerProperties* pAvailableLayers, uint32_t availableLayerCount, const char** ppLayers, uint32_t layerCount)
{
	uint32_t availableLayerIndex;
	for(uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
	{
		for(availableLayerIndex = 0; availableLayerIndex < availableLayerCount; ++availableLayerIndex)
		{
			if(strcmp(ppLayers[layerIndex], pAvailableLayers[availableLayerIndex].layerName) == 0) break;
		}

		if(availableLayerIndex == availableLayerCount) return false;
	}

	return true;
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
	VkExtensionProperties* pAvailableExtensions,
	uint32_t availableExtensionCount,
	const char** ppExtensions,
	uint32_t extensionCount,
	uint32_t requiredExtensionCount,
	bool* pOptionalExtensionAvaialble
)
{
	uint32_t finalCount = 0;
	uint32_t availableExtensionIndex;
	for(uint32_t extensionIndex = 0; extensionIndex < extensionCount; ++extensionIndex)
	{
		for(availableExtensionIndex = 0; availableExtensionIndex < availableExtensionCount; ++availableExtensionIndex)
		{
			if(strcmp(ppExtensions[extensionIndex], pAvailableExtensions[availableExtensionIndex].extensionName) == 0)
			{
				if(extensionIndex >= requiredExtensionCount) pOptionalExtensionAvaialble[extensionIndex - requiredExtensionCount] = true;

				break;
			}
		}

		if(availableExtensionIndex == availableExtensionCount)
		{
			if(extensionIndex < requiredExtensionCount) return 0;

			pOptionalExtensionAvaialble[extensionIndex - requiredExtensionCount] = false;
		}
		else ++finalCount;
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
	for(uint32_t i = 0; i < pCallbackData->objectCount; ++i) printf("|%s", pCallbackData->pObjects[i].pObjectName);

	// Message
	printf("] %s\n", pCallbackData->pMessage);

#ifdef _WIN32
	// Reset console color
	SetConsoleTextAttribute(console, consoleInfo.wAttributes);
#else
	printf("\x1b[0m");
#endif

	return VK_FALSE;
}
#endif

FrResult frCreateInstance(FrVulkanData* pVulkanData, const char* pName, uint32_t version)
{
#ifndef NDEBUG
	uint32_t availableLayerCount;
	if(vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	VkLayerProperties* pAvailableLayers = malloc(availableLayerCount * sizeof(VkLayerProperties));
	if(!pAvailableLayers) return FR_ERROR_OUT_OF_MEMORY;
	if(vkEnumerateInstanceLayerProperties(&availableLayerCount, pAvailableLayers) != VK_SUCCESS)
	{
		free(pAvailableLayers);
		return FR_ERROR_UNKNOWN;
	}

	const char* ppLayers[] = {"VK_LAYER_KHRONOS_validation"};
	const uint32_t layerCount = sizeof(ppLayers) / sizeof(ppLayers[0]);

	bool validationLayerAvailable = frLayersAvailable(pAvailableLayers, availableLayerCount, ppLayers, layerCount);
	if(!validationLayerAvailable) printf("Validation layers not found.\n");

	free(pAvailableLayers);
#endif

	uint32_t availableExtensionCount;
	if(vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	VkExtensionProperties* pAvailableExtensions = malloc(availableExtensionCount * sizeof(VkExtensionProperties));
	if(!pAvailableExtensions) return FR_ERROR_OUT_OF_MEMORY;
	if(vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, pAvailableExtensions) != VK_SUCCESS)
	{
		free(pAvailableExtensions);
		return FR_ERROR_UNKNOWN;
	}

	const char* ppExtensions[] = {
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
	const uint32_t extensionCount = sizeof(ppExtensions) / sizeof(ppExtensions[0]);
	const uint32_t requiredExtensionCount = 2;
	bool pOptionalExtensionsAvailable[1];

	const uint32_t finalExtensionCount = frExtensionsAvailable(
		pAvailableExtensions,
		availableExtensionCount,
		ppExtensions,
		extensionCount,
		requiredExtensionCount,
		pOptionalExtensionsAvailable
	);
	free(pAvailableExtensions);
	if(!finalExtensionCount) return FR_ERROR_UNKNOWN;


#ifndef NDEBUG
	pVulkanData->debugExtensionAvailable = pOptionalExtensionsAvailable[extensionCount - requiredExtensionCount - 1];
#endif

#ifndef NDEBUG
	if(!pVulkanData->debugExtensionAvailable && validationLayerAvailable)
	{
		if(vkEnumerateInstanceExtensionProperties("VK_LAYER_KHRONOS_validation", &availableExtensionCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
		pAvailableExtensions = malloc(availableExtensionCount * sizeof(VkExtensionProperties));
		if(!pAvailableExtensions) return FR_ERROR_OUT_OF_MEMORY;
		if(vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, pAvailableExtensions) != VK_SUCCESS)
		{
			free(pAvailableExtensions);
			return FR_ERROR_UNKNOWN;
		}

		pVulkanData->debugExtensionAvailable = frExtensionsAvailable(
			pAvailableExtensions,
			availableExtensionCount,
			ppExtensions + extensionCount - 1,
			1,
			1,
			NULL
		);

		free(pAvailableExtensions);
	}

	if(!pVulkanData->debugExtensionAvailable) printf("Debug extension not found.\n");
#endif

	const char** ppFinalExtensions = malloc(finalExtensionCount * sizeof(ppFinalExtensions[0]));
	if(!ppFinalExtensions) return FR_ERROR_OUT_OF_MEMORY;
	for(uint32_t i = 0, optionalIndex = requiredExtensionCount; i < extensionCount; ++i)
	{
		if(i < requiredExtensionCount) ppFinalExtensions[i] = ppExtensions[i];
		else if(pOptionalExtensionsAvailable[i - requiredExtensionCount])
		{
			ppFinalExtensions[optionalIndex] = ppExtensions[i];
			++optionalIndex;
		}
	}

#ifndef NDEBUG
	VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = frMessengerCallback
	};
#endif

	VkApplicationInfo applicationInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = pName,
		.applicationVersion = version,
		.pEngineName = "Fraus",
		.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0
	};

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &applicationInfo,
#ifndef NDEBUG
		.pNext = pVulkanData->debugExtensionAvailable ? &messengerCreateInfo : NULL,
		.enabledLayerCount = validationLayerAvailable ? layerCount : 0,
		.ppEnabledLayerNames = ppLayers,
#endif
		.enabledExtensionCount = finalExtensionCount,
		.ppEnabledExtensionNames = ppFinalExtensions
	};

	if(vkCreateInstance(&createInfo, NULL, &pVulkanData->instance) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	free(ppFinalExtensions);

	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkDestroyInstance)
#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkCreateDebugUtilsMessengerEXT)
		FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkDestroyDebugUtilsMessengerEXT)
		FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkSetDebugUtilsObjectNameEXT)
	}
#endif
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkGetPhysicalDeviceSurfaceSupportKHR)
#ifdef _WIN32
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkCreateWin32SurfaceKHR)
#else
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkCreateXlibSurfaceKHR)
#endif
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkDestroySurfaceKHR)
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkEnumeratePhysicalDevices)
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkGetPhysicalDeviceProperties)
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkGetPhysicalDeviceFeatures)
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkGetPhysicalDeviceQueueFamilyProperties)
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkGetPhysicalDeviceMemoryProperties)
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkGetPhysicalDeviceFormatProperties)
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkGetPhysicalDeviceSurfaceFormatsKHR)
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkGetPhysicalDeviceSurfacePresentModesKHR)
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkGetDeviceProcAddr)
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkEnumerateDeviceLayerProperties)
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkEnumerateDeviceExtensionProperties)
	FR_LOAD_INSTANCE_PFN(pVulkanData->functions, pVulkanData->instance, vkCreateDevice)

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		messengerCreateInfo.pUserData = &pVulkanData->instance;
		if(pVulkanData->functions.vkCreateDebugUtilsMessengerEXT(pVulkanData->instance, &messengerCreateInfo, NULL, &pVulkanData->messenger) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

FrResult frChoosePhysicalDevice(FrVulkanData* pVulkanData)
{
	uint32_t physicalDeviceCount;
	if(pVulkanData->functions.vkEnumeratePhysicalDevices(pVulkanData->instance, &physicalDeviceCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	VkPhysicalDevice* pPhysicalDevices = malloc(physicalDeviceCount * sizeof(VkPhysicalDevice));
	if(!pPhysicalDevices) return FR_ERROR_OUT_OF_MEMORY;
	if(pVulkanData->functions.vkEnumeratePhysicalDevices(pVulkanData->instance, &physicalDeviceCount, pPhysicalDevices) != VK_SUCCESS)
	{
		free(pPhysicalDevices);
		return FR_ERROR_UNKNOWN;
	}

	for(uint32_t i = 0; i < physicalDeviceCount; ++i)
	{
		uint32_t queueFamilyPropertyCount;
		pVulkanData->functions.vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevices[i], &queueFamilyPropertyCount, NULL);
		VkQueueFamilyProperties* pQueueFamilyProperties = malloc(queueFamilyPropertyCount * sizeof(VkQueueFamilyProperties));
		if(!pQueueFamilyProperties)
		{
			free(pPhysicalDevices);
			return FR_ERROR_UNKNOWN;
		}
		pVulkanData->functions.vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevices[i], &queueFamilyPropertyCount, pQueueFamilyProperties);

		for(uint32_t j = 0; j < queueFamilyPropertyCount; ++j)
		{
			VkBool32 surfaceSupported;
			if(pVulkanData->functions.vkGetPhysicalDeviceSurfaceSupportKHR(pPhysicalDevices[i], j, pVulkanData->surface, &surfaceSupported) != VK_SUCCESS)
			{
				free(pQueueFamilyProperties);
				free(pPhysicalDevices);
				return FR_ERROR_UNKNOWN;
			}

			if(pQueueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT && surfaceSupported)
			{
				pVulkanData->physicalDevice = pPhysicalDevices[i];
				pVulkanData->queueFamily = j;

				VkPhysicalDeviceProperties properties;
				pVulkanData->functions.vkGetPhysicalDeviceProperties(pVulkanData->physicalDevice, &properties);

				VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
				if (counts & VK_SAMPLE_COUNT_64_BIT) pVulkanData->msaaSamples = VK_SAMPLE_COUNT_64_BIT;
				else if(counts & VK_SAMPLE_COUNT_32_BIT) pVulkanData->msaaSamples = VK_SAMPLE_COUNT_32_BIT;
				else if(counts & VK_SAMPLE_COUNT_16_BIT) pVulkanData->msaaSamples = VK_SAMPLE_COUNT_16_BIT;
				else if(counts & VK_SAMPLE_COUNT_8_BIT) pVulkanData->msaaSamples = VK_SAMPLE_COUNT_8_BIT;
				else if(counts & VK_SAMPLE_COUNT_4_BIT) pVulkanData->msaaSamples = VK_SAMPLE_COUNT_4_BIT;
				else if(counts & VK_SAMPLE_COUNT_2_BIT) pVulkanData->msaaSamples = VK_SAMPLE_COUNT_2_BIT;
				else pVulkanData->msaaSamples = VK_SAMPLE_COUNT_1_BIT;

				free(pQueueFamilyProperties);
				free(pPhysicalDevices);

				return FR_SUCCESS;
			}
		}

		free(pQueueFamilyProperties);
	}

	free(pPhysicalDevices);

	return FR_ERROR_UNKNOWN;
}

FrResult frCreateSurface(FrVulkanData* pVulkanData)
{
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = GetModuleHandle(NULL),
		.hwnd = pVulkanData->pApplication->window.handle
	};
	if(pVulkanData->functions.vkCreateWin32SurfaceKHR(pVulkanData->instance, &createInfo, NULL, &pVulkanData->surface) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
#else
	VkXlibSurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
		.dpy = pVulkanData->pApplication->window.pDisplay,
		.window = pVulkanData->pApplication->window.window
	};
	if(pVulkanData->functions.vkCreateXlibSurfaceKHR(pVulkanData->instance, &createInfo, NULL, &pVulkanData->surface) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
#endif

	return FR_SUCCESS;
}

FrResult frCreateDevice(FrVulkanData* pVulkanData)
{
	float priority = 1.f;

	uint32_t availableExtensionCount;
	if(pVulkanData->functions.vkEnumerateDeviceExtensionProperties(pVulkanData->physicalDevice, NULL, &availableExtensionCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	VkExtensionProperties* pAvailableExtensions = malloc(availableExtensionCount * sizeof(VkExtensionProperties));
	if(!pAvailableExtensions) return FR_ERROR_UNKNOWN;
	if(pVulkanData->functions.vkEnumerateDeviceExtensionProperties(pVulkanData->physicalDevice, NULL, &availableExtensionCount, pAvailableExtensions) != VK_SUCCESS)
	{
		free(pAvailableExtensions);
		return FR_ERROR_UNKNOWN;
	}

	const char* ppExtensions[] = {
		// Required
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	const uint32_t extensionCount = sizeof(ppExtensions) / sizeof(ppExtensions[0]);
	const uint32_t requiredExtensionCount = 1;
	bool* pOptionalExtensionsAvailable = NULL;

	const uint32_t finalExtensionCount = frExtensionsAvailable(
		pAvailableExtensions,
		availableExtensionCount,
		ppExtensions,
		extensionCount,
		requiredExtensionCount,
		pOptionalExtensionsAvailable
	);
	free(pAvailableExtensions);
	if(!finalExtensionCount) return FR_ERROR_UNKNOWN;

	const char** ppFinalExtensions = malloc(finalExtensionCount * sizeof(ppFinalExtensions[0]));
	if(!ppFinalExtensions) return FR_ERROR_OUT_OF_MEMORY;
	for(uint32_t i = 0, optionalIndex = requiredExtensionCount; i < extensionCount; ++i)
	{
		if(i < requiredExtensionCount) ppFinalExtensions[i] = ppExtensions[i];
		else if(pOptionalExtensionsAvailable[i - requiredExtensionCount])
		{
			ppFinalExtensions[optionalIndex] = ppExtensions[i];
			++optionalIndex;
		}
	}

	VkDeviceQueueCreateInfo queueCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = pVulkanData->queueFamily,
		.queueCount = 1,
		.pQueuePriorities = &priority
	};

	VkPhysicalDeviceFeatures features, wantedFeatures = {VK_FALSE};
	pVulkanData->functions.vkGetPhysicalDeviceFeatures(pVulkanData->physicalDevice, &features);
	wantedFeatures.samplerAnisotropy = features.samplerAnisotropy;
	wantedFeatures.sampleRateShading = features.sampleRateShading;

	VkDeviceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = finalExtensionCount,
		.ppEnabledExtensionNames = ppFinalExtensions,
		.pEnabledFeatures = &wantedFeatures
	};

// Load device layers for compatibility
#ifndef NDEBUG
	uint32_t availableLayerCount;
	if(pVulkanData->functions.vkEnumerateDeviceLayerProperties(pVulkanData->physicalDevice, &availableLayerCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	VkLayerProperties* pAvailableLayers = malloc(availableLayerCount * sizeof(VkLayerProperties));
	if(!pAvailableLayers) return FR_ERROR_OUT_OF_MEMORY;
	if(pVulkanData->functions.vkEnumerateDeviceLayerProperties(pVulkanData->physicalDevice, &availableLayerCount, pAvailableLayers) != VK_SUCCESS)
	{
		free(ppFinalExtensions);
		free(pAvailableLayers);
		return FR_ERROR_UNKNOWN;
	}

	const char* ppLayers[] = {"VK_LAYER_KHRONOS_validation"};
	const uint32_t layerCount = sizeof(ppLayers) / sizeof(ppLayers[0]);

	if(frLayersAvailable(pAvailableLayers, availableLayerCount, ppLayers, layerCount))
	{
		createInfo.enabledLayerCount = layerCount;
		createInfo.ppEnabledLayerNames = ppLayers;
	}

	free(pAvailableLayers);
#endif

	if(pVulkanData->functions.vkCreateDevice(pVulkanData->physicalDevice, &createInfo, NULL, &pVulkanData->device) != VK_SUCCESS)
	{
		free(ppFinalExtensions);
		return FR_ERROR_UNKNOWN;
	}

	free(ppFinalExtensions);

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_INSTANCE,
			.objectHandle = (uint64_t)pVulkanData->instance,
			.pObjectName = "Instance"
		};
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		// The two next crash for some reason
		nameInfo.objectType = VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT;
		nameInfo.objectHandle = (uint64_t)pVulkanData->messenger;
		nameInfo.pObjectName = "Messenger";
		// if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectType = VK_OBJECT_TYPE_SURFACE_KHR;
		nameInfo.objectHandle = (uint64_t)pVulkanData->surface;
		nameInfo.pObjectName = "Surface";
		// if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectType = VK_OBJECT_TYPE_PHYSICAL_DEVICE;
		nameInfo.objectHandle = (uint64_t)pVulkanData->physicalDevice;
		nameInfo.pObjectName = "Physical device";
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectType = VK_OBJECT_TYPE_DEVICE;
		nameInfo.objectHandle = (uint64_t)pVulkanData->device;
		nameInfo.pObjectName = "Device";
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroyDevice)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkGetDeviceQueue)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateSwapchainKHR)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroySwapchainKHR)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkGetSwapchainImagesKHR)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateImage)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroyImage)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateImageView)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroyImageView)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateSampler)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroySampler)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateFramebuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroyFramebuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateRenderPass)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroyRenderPass)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateDescriptorSetLayout)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroyDescriptorSetLayout)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateShaderModule)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroyShaderModule)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreatePipelineLayout)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroyPipelineLayout)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateGraphicsPipelines)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroyPipeline)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateBuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroyBuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkGetBufferMemoryRequirements)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkGetImageMemoryRequirements)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkAllocateMemory)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkFreeMemory)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkBindBufferMemory)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkBindImageMemory)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkMapMemory)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkUnmapMemory)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateSemaphore)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroySemaphore)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateFence)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroyFence)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkWaitForFences)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkResetFences)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateCommandPool)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroyCommandPool)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCreateDescriptorPool)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDestroyDescriptorPool)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkAllocateDescriptorSets)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkUpdateDescriptorSets)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkAllocateCommandBuffers)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkFreeCommandBuffers)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkResetCommandBuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkResetCommandPool)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkAcquireNextImageKHR)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkQueuePresentKHR)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkQueueSubmit)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkQueueWaitIdle)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkBeginCommandBuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkEndCommandBuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdExecuteCommands)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdBeginRenderPass)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdNextSubpass)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdEndRenderPass)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdBindPipeline)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdPipelineBarrier)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdCopyBuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdCopyBufferToImage)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdBlitImage)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdBindVertexBuffers)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdBindIndexBuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdBindDescriptorSets)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdPushConstants)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdSetViewport)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdSetScissor)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdDraw)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkCmdDrawIndexed)
	FR_LOAD_DEVICE_PFN(pVulkanData->functions, pVulkanData->device, vkDeviceWaitIdle)

	pVulkanData->functions.vkGetDeviceQueue(pVulkanData->device, pVulkanData->queueFamily, 0, &pVulkanData->queue);

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_QUEUE,
			.objectHandle = (uint64_t)pVulkanData->queue,
			.pObjectName = "Queue"
		};
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

FrResult frCreateSwapchain(FrVulkanData* pVulkanData)
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	if(pVulkanData->functions.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pVulkanData->physicalDevice, pVulkanData->surface, &surfaceCapabilities) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	pVulkanData->extent = surfaceCapabilities.currentExtent;

	uint32_t surfaceFormatCount;
	if(pVulkanData->functions.vkGetPhysicalDeviceSurfaceFormatsKHR(pVulkanData->physicalDevice, pVulkanData->surface, &surfaceFormatCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	VkSurfaceFormatKHR* pSurfaceFormats = malloc(surfaceFormatCount * sizeof(VkSurfaceFormatKHR));
	if(!pSurfaceFormats) return FR_ERROR_OUT_OF_MEMORY;
	if(pVulkanData->functions.vkGetPhysicalDeviceSurfaceFormatsKHR(pVulkanData->physicalDevice, pVulkanData->surface, &surfaceFormatCount, pSurfaceFormats) != VK_SUCCESS)
	{
		free(pSurfaceFormats);
		return FR_ERROR_UNKNOWN;
	}
	uint32_t surfaceFormatIndex = 0;
	for(uint32_t i = 0; i < surfaceFormatCount; ++i)
	{
		if(pSurfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && pSurfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			surfaceFormatIndex = i;
			break;
		}
	}
	pVulkanData->format = pSurfaceFormats[surfaceFormatIndex].format;

	uint32_t presentModeCount;
	if(pVulkanData->functions.vkGetPhysicalDeviceSurfacePresentModesKHR(pVulkanData->physicalDevice, pVulkanData->surface, &presentModeCount, NULL) != VK_SUCCESS)
	{
		free(pSurfaceFormats);
		return FR_ERROR_UNKNOWN;
	}
	VkPresentModeKHR* pPresentModes = malloc(presentModeCount * sizeof(VkPresentModeKHR));
	if(!pPresentModes)
	{
		free(pSurfaceFormats);
		return FR_ERROR_UNKNOWN;
	}
	if(pVulkanData->functions.vkGetPhysicalDeviceSurfacePresentModesKHR(pVulkanData->physicalDevice, pVulkanData->surface, &presentModeCount, pPresentModes) != VK_SUCCESS)
	{
		free(pSurfaceFormats);
		free(pPresentModes);
		return FR_ERROR_UNKNOWN;
	}

	uint32_t presentModeIndex;
	uint32_t fifoIndex = presentModeCount;
	for(presentModeIndex = 0; presentModeIndex < presentModeCount; ++presentModeIndex)
	{
		if(pPresentModes[presentModeIndex] == VK_PRESENT_MODE_FIFO_KHR)
		{
			fifoIndex = presentModeIndex;
			continue;
		}

		if(pPresentModes[presentModeIndex] == VK_PRESENT_MODE_MAILBOX_KHR)
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

	VkSwapchainCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = pVulkanData->surface,
		.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.minImageCount < surfaceCapabilities.maxImageCount ? 1 : 0),
		.imageFormat = pSurfaceFormats[surfaceFormatIndex].format,
		.imageColorSpace = pSurfaceFormats[surfaceFormatIndex].colorSpace,
		.imageExtent = surfaceCapabilities.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1, // useless with exclusive
		.pQueueFamilyIndices = &pVulkanData->queueFamily, // useless with exclusive
		.preTransform = surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = pPresentModes[presentModeIndex],
		.clipped = VK_TRUE,
		.oldSwapchain = pVulkanData->swapchain
	};

	if(pVulkanData->functions.vkCreateSwapchainKHR(pVulkanData->device, &createInfo, NULL, &pVulkanData->swapchain) != VK_SUCCESS)
	{
		free(pSurfaceFormats);
		free(pPresentModes);
		return EXIT_FAILURE;
	}

	free(pSurfaceFormats);
	free(pPresentModes);

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR,
			.objectHandle = (uint64_t)pVulkanData->swapchain,
			.pObjectName = "Swapchain"
		};
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	if(pVulkanData->functions.vkGetSwapchainImagesKHR(pVulkanData->device, pVulkanData->swapchain, &pVulkanData->imageCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	VkImage* pNewImages = realloc(pVulkanData->pImages, pVulkanData->imageCount * sizeof(VkImage));
	if(!pNewImages) return FR_ERROR_UNKNOWN;
	pVulkanData->pImages = pNewImages;
	if(pVulkanData->functions.vkGetSwapchainImagesKHR(pVulkanData->device, pVulkanData->swapchain, &pVulkanData->imageCount, pVulkanData->pImages) != VK_SUCCESS)
	{
		free(pVulkanData->pImages);
		pVulkanData->pImages = NULL;
		return FR_ERROR_UNKNOWN;
	}

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_IMAGE
		};
		char name[64];
		for(uint32_t imageIndex = 0; imageIndex < pVulkanData->imageCount; ++imageIndex)
		{
			if(snprintf(name, sizeof(name), "Swapchain image %u", imageIndex) < 0) return FR_ERROR_UNKNOWN;
			nameInfo.objectHandle = (uint64_t)pVulkanData->pImages[imageIndex];
			nameInfo.pObjectName = name;
			if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
		}
	}
#endif

	VkImageView* pNewImageViews = realloc(pVulkanData->pImageViews, pVulkanData->imageCount * sizeof(VkImageView));
	if(!pNewImageViews)
	{
		free(pVulkanData->pImages);
		pVulkanData->pImages = NULL;
		return FR_ERROR_UNKNOWN;
	}
	pVulkanData->pImageViews = pNewImageViews;
	for(uint32_t imageIndex = 0; imageIndex < pVulkanData->imageCount; ++imageIndex)
	{
		if(frCreateImageView(pVulkanData, pVulkanData->pImages[imageIndex], pVulkanData->format, VK_IMAGE_ASPECT_COLOR_BIT, 1, &pVulkanData->pImageViews[imageIndex]) != FR_SUCCESS)
		{
			for(uint32_t j = 0; j < imageIndex; ++j)
			{
				pVulkanData->functions.vkDestroyImageView(pVulkanData->device, pVulkanData->pImageViews[j], NULL);
			}
			free(pVulkanData->pImageViews);
			free(pVulkanData->pImages);
			pVulkanData->pImageViews = NULL;
			pVulkanData->pImages = NULL;
			return FR_ERROR_UNKNOWN;
		}

#ifndef NDEBUG
		if(pVulkanData->debugExtensionAvailable)
		{
			char name[64];
			if(snprintf(name, sizeof(name), "Swapchain image view %u", imageIndex) < 0) return FR_ERROR_UNKNOWN;
			VkDebugUtilsObjectNameInfoEXT nameInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
				.objectHandle = (uint64_t)pVulkanData->pImageViews[imageIndex],
				.pObjectName = name
			};
			if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
		}
#endif
	}

	return FR_SUCCESS;
}

FrResult frCreateRenderPass(FrVulkanData* pVulkanData)
{
	VkAttachmentDescription pAttachments[] = {
		{
			.format = pVulkanData->format,
			.samples = pVulkanData->msaaSamples,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		},
		{
			.format = VK_FORMAT_D24_UNORM_S8_UINT,
			.samples = pVulkanData->msaaSamples,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		},
		{
			.format = pVulkanData->format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		}
	};

	VkAttachmentReference colorAttachmentReference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkAttachmentReference depthAttachmentReference = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
	VkAttachmentReference resolveAttachmentReference = {
		.attachment = 2,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pResolveAttachments = &resolveAttachmentReference,
		.pColorAttachments = &colorAttachmentReference,
		.pDepthStencilAttachment = &depthAttachmentReference
	};

	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	};

	VkRenderPassCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = sizeof(pAttachments) / sizeof(pAttachments[0]),
		.pAttachments = pAttachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	if(pVulkanData->functions.vkCreateRenderPass(pVulkanData->device, &createInfo, NULL, &pVulkanData->renderPass) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_RENDER_PASS,
			.objectHandle = (uint64_t)pVulkanData->renderPass,
			.pObjectName = "Render pass"
		};
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

FrResult frCreateFramebuffers(FrVulkanData* pVulkanData)
{
	VkFramebuffer* pNewFramebuffers = realloc(pVulkanData->pFramebuffers, pVulkanData->imageCount * sizeof(VkFramebuffer));
	if(!pNewFramebuffers)
	{
		free(pVulkanData->pFramebuffers);
		return FR_ERROR_UNKNOWN;
	}
	pVulkanData->pFramebuffers = pNewFramebuffers;

	for(uint32_t imageIndex = 0; imageIndex < pVulkanData->imageCount; ++imageIndex)
	{
		VkImageView attachments[] = {
			pVulkanData->colorImageView,
			pVulkanData->depthImageView,
			pVulkanData->pImageViews[imageIndex]
		};
		VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = pVulkanData->renderPass,
			.attachmentCount = sizeof(attachments) / sizeof(VkImageView),
			.pAttachments = attachments,
			.width = pVulkanData->extent.width,
			.height = pVulkanData->extent.height,
			.layers = 1
		};
		if(pVulkanData->functions.vkCreateFramebuffer(pVulkanData->device, &framebufferInfo, NULL, &pVulkanData->pFramebuffers[imageIndex]) != VK_SUCCESS)
		{
			for(uint32_t j = 0; j < imageIndex; ++j)
			{
				pVulkanData->functions.vkDestroyFramebuffer(pVulkanData->device, pVulkanData->pFramebuffers[j], NULL);
			}

			free(pVulkanData->pFramebuffers);
			pVulkanData->pFramebuffers = NULL;
			return FR_ERROR_UNKNOWN;
		}

#ifndef NDEBUG
		if(pVulkanData->debugExtensionAvailable)
		{
			char name[64];
			if(snprintf(name, sizeof(name), "Framebuffer %u", imageIndex) < 0) return FR_ERROR_UNKNOWN;
			VkDebugUtilsObjectNameInfoEXT nameInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_FRAMEBUFFER,
				.objectHandle = (uint64_t)pVulkanData->pFramebuffers[imageIndex],
				.pObjectName = name
			};
			if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
		}
#endif
	}

	return FR_SUCCESS;
}

FrResult frCreateShaderModule(FrVulkanData* pVulkanData, const char* pPath, VkShaderModule* pShaderModule)
{
	// Prepare create info
	VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO
	};

	// Open file
	FILE* file = fopen(pPath, "rb");
	if(!file) return FR_ERROR_FILE_NOT_FOUND;
	
	// Get file size
	fseek(file, 0, SEEK_END);
	createInfo.codeSize = ftell(file);

	// SPIR-V code must contain 32-bit words
	if(createInfo.codeSize % sizeof(uint32_t) != 0)
	{
		fclose(file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Allocate memory for SPIR-V code
	uint32_t* pCode = malloc(createInfo.codeSize);
	if(!pCode)
	{
		fclose(file);
		return FR_ERROR_OUT_OF_MEMORY;
	}

	// Read SPIR-V code
	fseek(file, 0, SEEK_SET);
	if(fread(pCode, sizeof(uint32_t), createInfo.codeSize / sizeof(uint32_t), file) != createInfo.codeSize / sizeof(uint32_t))
	{
		free(pCode);
		fclose(file);
		return FR_ERROR_UNKNOWN;
	}
	createInfo.pCode = pCode;

	// Close file
	fclose(file);
	
	// Create shader module
	if(pVulkanData->functions.vkCreateShaderModule(pVulkanData->device, &createInfo, NULL, pShaderModule) != VK_SUCCESS)
	{
		free(pCode);
		return FR_ERROR_UNKNOWN;
	}

	// Free SPIR-V code
	free(pCode);

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_SHADER_MODULE,
			.objectHandle = (uint64_t)*pShaderModule,
			.pObjectName = pPath[strlen(pPath) - 1] == 't' ? "Vertex shader module" : "Fragment shader module" // .vert / .frag
		};
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

FrResult frCreateGraphicsPipeline(
	FrVulkanData* pVulkanData,
	const char* pVertexShaderPath,
	const char* pFragmentShaderPath,
	VkDescriptorSetLayoutBinding* pDescriptorSetLayoutBindings,
	uint32_t descriptorSetLayoutBindingCount
)
{
	if(frPushBackPipelineVector(&pVulkanData->graphicsPipelines, (FrPipeline){0}) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	// Shader stages
	VkShaderModule vertexModule, fragmentModule;
	if(frCreateShaderModule(pVulkanData, pVertexShaderPath, &vertexModule) != FR_SUCCESS) return FR_ERROR_UNKNOWN;
	if(frCreateShaderModule(pVulkanData, pFragmentShaderPath, &fragmentModule) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	VkPipelineShaderStageCreateInfo stageInfos[] = {
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
	VkVertexInputBindingDescription binding = {
		.binding = 0,
		.stride = sizeof(FrVertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	VkVertexInputAttributeDescription attributes[] = {
		{
			.binding = 0,
			.location = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(FrVertex, position)
		},
		{
			.binding = 0,
			.location = 1,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(FrVertex, textureCoordinates)
		},
		{
			.binding = 0,
			.location = 2,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(FrVertex, normal)
		}
	};

	VkPipelineVertexInputStateCreateInfo vertexInput = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &binding,
		.vertexAttributeDescriptionCount = sizeof(attributes) / sizeof(VkVertexInputAttributeDescription),
		.pVertexAttributeDescriptions = attributes
	};

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	// Tessellation
	VkPipelineTessellationStateCreateInfo tessellation = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.patchControlPoints = 1
	};

	// Viewport
	VkPipelineViewportStateCreateInfo viewport = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1
	};

	// Rasterization
	VkPipelineRasterizationStateCreateInfo rasterization = {
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
	pVulkanData->functions.vkGetPhysicalDeviceFeatures(pVulkanData->physicalDevice, &features);
	VkPipelineMultisampleStateCreateInfo multisample = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = pVulkanData->msaaSamples,
		.sampleShadingEnable = features.sampleRateShading,
		.minSampleShading = 1.f
	};

	// Depth stencil
	VkPipelineDepthStencilStateCreateInfo depthStencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE
	};

	// Color blend
	VkPipelineColorBlendAttachmentState colorAttachment = {
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo colorBlend = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment
	};

	// Dynamic
	VkDynamicState dynamics[] = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};

	VkPipelineDynamicStateCreateInfo dynamic = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = sizeof(dynamics) / sizeof(VkDynamicState),
		.pDynamicStates = dynamics
	};

	pVulkanData->graphicsPipelines.pData[pVulkanData->graphicsPipelines.size - 1].descriptorTypeCount = descriptorSetLayoutBindingCount;
	pVulkanData->graphicsPipelines.pData[pVulkanData->graphicsPipelines.size - 1].pDescriptorTypes = malloc(descriptorSetLayoutBindingCount * sizeof(VkDescriptorType));
	if(!pVulkanData->graphicsPipelines.pData[pVulkanData->graphicsPipelines.size - 1].pDescriptorTypes) return FR_ERROR_OUT_OF_MEMORY;
	for(uint32_t i = 0; i < descriptorSetLayoutBindingCount; ++i)
	{
		pVulkanData->graphicsPipelines.pData[pVulkanData->graphicsPipelines.size - 1].pDescriptorTypes[i] = pDescriptorSetLayoutBindings[i].descriptorType;
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = descriptorSetLayoutBindingCount,
		.pBindings = pDescriptorSetLayoutBindings
	};
	if(pVulkanData->functions.vkCreateDescriptorSetLayout(
		pVulkanData->device,
		&descriptorSetLayoutInfo,
		NULL,
		&pVulkanData->graphicsPipelines.pData[pVulkanData->graphicsPipelines.size - 1].descriptorSetLayout
	) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		char name[64];
		if(snprintf(name, sizeof(name), "Descriptor set layout %zu", pVulkanData->graphicsPipelines.size - 1) < 0) return FR_ERROR_UNKNOWN;
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
			.objectHandle = (uint64_t)pVulkanData->graphicsPipelines.pData[pVulkanData->graphicsPipelines.size - 1].descriptorSetLayout,
			.pObjectName = name
		};
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	VkPushConstantRange pushConstantRange = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(pVulkanData->objects.pData[0].transformation)
	};

	// Layout
	VkPipelineLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &pVulkanData->graphicsPipelines.pData[pVulkanData->graphicsPipelines.size - 1].descriptorSetLayout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange
	};

	if(pVulkanData->functions.vkCreatePipelineLayout(
		pVulkanData->device,
		&layoutInfo,
		NULL,
		&pVulkanData->graphicsPipelines.pData[pVulkanData->graphicsPipelines.size - 1].pipelineLayout
	) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		char name[64];
		if(snprintf(name, sizeof(name), "Pipeline layout %zu", pVulkanData->graphicsPipelines.size - 1) < 0) return FR_ERROR_UNKNOWN;
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT,
			.objectHandle = (uint64_t)pVulkanData->graphicsPipelines.pData[pVulkanData->graphicsPipelines.size - 1].pipelineLayout,
			.pObjectName = name
		};
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	// Pipeline
	VkGraphicsPipelineCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = sizeof(stageInfos) / sizeof(VkPipelineShaderStageCreateInfo),
		.pStages = stageInfos,
		.pVertexInputState = &vertexInput,
		.pInputAssemblyState = &inputAssembly,
		.pTessellationState = &tessellation,
		.pViewportState = &viewport,
		.pRasterizationState = &rasterization,
		.pMultisampleState = &multisample,
		.pDepthStencilState = &depthStencil,
		.pColorBlendState = &colorBlend,
		.pDynamicState = &dynamic,
		.layout = pVulkanData->graphicsPipelines.pData[pVulkanData->graphicsPipelines.size - 1].pipelineLayout,
		.renderPass = pVulkanData->renderPass,
		.subpass = 0
	};

	if(pVulkanData->functions.vkCreateGraphicsPipelines(
		pVulkanData->device,
		VK_NULL_HANDLE,
		1,
		&createInfo,
		NULL,
		&pVulkanData->graphicsPipelines.pData[pVulkanData->graphicsPipelines.size - 1].pipeline
	) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	pVulkanData->functions.vkDestroyShaderModule(pVulkanData->device, vertexModule, NULL);
	pVulkanData->functions.vkDestroyShaderModule(pVulkanData->device, fragmentModule, NULL);

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		char name[64];
		if(snprintf(name, sizeof(name), "Graphics pipeline %zu", pVulkanData->graphicsPipelines.size - 1) < 0) return FR_ERROR_UNKNOWN;
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_PIPELINE,
			.objectHandle = (uint64_t)pVulkanData->graphicsPipelines.pData[pVulkanData->graphicsPipelines.size - 1].pipeline,
			.pObjectName = name
		};
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

FrResult frCreateUniformBuffer(FrVulkanData* pVulkanData, VkDeviceSize size)
{
	if(frPushBackUniformBufferVector(&pVulkanData->uniformBuffers, (FrUniformBuffer){0}) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	pVulkanData->uniformBuffers.pData[pVulkanData->uniformBuffers.size - 1].buffersSize = size;

	// Create uniform buffers
	for(uint32_t frameIndex = 0; frameIndex < FR_FRAMES_IN_FLIGHT; ++frameIndex)
	{
		if(frCreateBuffer(
			pVulkanData,
			size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&pVulkanData->uniformBuffers.pData[pVulkanData->uniformBuffers.size - 1].buffers[frameIndex],
			&pVulkanData->uniformBuffers.pData[pVulkanData->uniformBuffers.size - 1].bufferMemories[frameIndex]
		) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

		if(pVulkanData->functions.vkMapMemory(pVulkanData->device, pVulkanData->uniformBuffers.pData[pVulkanData->uniformBuffers.size - 1].bufferMemories[frameIndex], 0, size, 0, &pVulkanData->uniformBuffers.pData[pVulkanData->uniformBuffers.size - 1].pBufferDatas[frameIndex]) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

#ifndef NDEBUG
		if(pVulkanData->debugExtensionAvailable)
		{
			char name[64];
			if(snprintf(name, sizeof(name), "Uniform buffer %zu %u", pVulkanData->uniformBuffers.size - 1, frameIndex) < 0) return FR_ERROR_UNKNOWN;
			VkDebugUtilsObjectNameInfoEXT nameInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_BUFFER,
				.objectHandle = (uint64_t)pVulkanData->uniformBuffers.pData[pVulkanData->uniformBuffers.size - 1].buffers[frameIndex],
				.pObjectName = name
			};
			if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

			if(snprintf(name, sizeof(name), "Uniform buffer memory %zu %u", pVulkanData->uniformBuffers.size - 1, frameIndex) < 0) return FR_ERROR_UNKNOWN;
			nameInfo.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
			nameInfo.objectHandle = (uint64_t)pVulkanData->uniformBuffers.pData[pVulkanData->uniformBuffers.size - 1].bufferMemories[frameIndex];
			if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
		}
#endif
	}

	return FR_SUCCESS;
}

FrResult frCreateSampler(FrVulkanData* pVulkanData)
{
	VkPhysicalDeviceFeatures features;
	pVulkanData->functions.vkGetPhysicalDeviceFeatures(pVulkanData->physicalDevice, &features);
	VkPhysicalDeviceProperties properties;
	pVulkanData->functions.vkGetPhysicalDeviceProperties(pVulkanData->physicalDevice, &properties);
	VkSamplerCreateInfo samplerCreateInfo = {
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
	if(pVulkanData->functions.vkCreateSampler(pVulkanData->device, &samplerCreateInfo, NULL, &pVulkanData->textureSampler) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_SAMPLER,
			.objectHandle = (uint64_t)pVulkanData->textureSampler,
			.pObjectName = "Texture sampler"
		};
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

FrResult frCreateColorImage(FrVulkanData* pVulkanData)
{
	if(frCreateImage(pVulkanData, pVulkanData->extent.width, pVulkanData->extent.height, 1, pVulkanData->msaaSamples, pVulkanData->format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &pVulkanData->colorImage, &pVulkanData->colorImageMemory) != FR_SUCCESS) return FR_ERROR_UNKNOWN;
	if(frCreateImageView(pVulkanData, pVulkanData->colorImage, pVulkanData->format, VK_IMAGE_ASPECT_COLOR_BIT, 1, &pVulkanData->colorImageView) != FR_SUCCESS)
	{
		pVulkanData->functions.vkDestroyImage(pVulkanData->device, pVulkanData->colorImage, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, pVulkanData->colorImageMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_IMAGE,
			.objectHandle = (uint64_t)pVulkanData->colorImage,
			.pObjectName = "Color image"
		};
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
		nameInfo.objectHandle = (uint64_t)pVulkanData->colorImageMemory;
		nameInfo.pObjectName = "Color image memory";
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
		nameInfo.objectHandle = (uint64_t)pVulkanData->colorImageView;
		nameInfo.pObjectName = "Color image view";
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

FrResult frCreateDepthImage(FrVulkanData* pVulkanData)
{
	VkFormatProperties properties;
	pVulkanData->functions.vkGetPhysicalDeviceFormatProperties(pVulkanData->physicalDevice, VK_FORMAT_D24_UNORM_S8_UINT, &properties);
	if(!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) return FR_ERROR_UNKNOWN;

	if(frCreateImage(
		pVulkanData,
		pVulkanData->extent.width,
		pVulkanData->extent.height,
		1,
		pVulkanData->msaaSamples,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&pVulkanData->depthImage,
		&pVulkanData->depthImageMemory
	) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	if(frCreateImageView(pVulkanData, pVulkanData->depthImage, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, 1, &pVulkanData->depthImageView) != FR_SUCCESS)
	{
		pVulkanData->functions.vkDestroyImage(pVulkanData->device, pVulkanData->depthImage, NULL);
		pVulkanData->functions.vkFreeMemory(pVulkanData->device, pVulkanData->depthImageMemory, NULL);
		return FR_ERROR_UNKNOWN;
	}

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_IMAGE,
			.objectHandle = (uint64_t)pVulkanData->depthImage,
			.pObjectName = "Depth image"
		};
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
		nameInfo.objectHandle = (uint64_t)pVulkanData->depthImageMemory;
		nameInfo.pObjectName = "Depth image memory";
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
		nameInfo.objectHandle = (uint64_t)pVulkanData->depthImageView;
		nameInfo.pObjectName = "Depth image view";
		if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

static FrResult frRecordCommandBuffer(FrVulkanData* pVulkanData, uint32_t threadIndex)
{
	const uint32_t objectCount = (uint32_t)pVulkanData->objects.size / pVulkanData->threadCount
		+ ((uint32_t)pVulkanData->objects.size % pVulkanData->threadCount > threadIndex ? 1 : 0);

	const uint32_t commandBufferIndex = pVulkanData->frameInFlightIndex * pVulkanData->threadCount + threadIndex;

	const uint32_t offset = (uint32_t)pVulkanData->objects.size % pVulkanData->threadCount > threadIndex ? threadIndex : (uint32_t)pVulkanData->objects.size % pVulkanData->threadCount;

	const uint32_t firstIndex = (uint32_t)pVulkanData->objects.size / pVulkanData->threadCount + offset;

	VkCommandBufferInheritanceInfo inherintanceInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.renderPass = pVulkanData->renderPass,
		.subpass = 0,
		.framebuffer = pVulkanData->pFramebuffers[pVulkanData->imageIndex],
		.occlusionQueryEnable = VK_FALSE,
		.queryFlags = 0,
	};
	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
		.pInheritanceInfo = &inherintanceInfo
	};
	if(pVulkanData->functions.vkBeginCommandBuffer(pVulkanData->pSecondaryCommandBuffers[commandBufferIndex], &beginInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	const VkViewport viewport = {
		.x = 0.f,
		.y = 0.f,
		.width = (float)pVulkanData->extent.width,
		.height = (float)pVulkanData->extent.height,
		.minDepth = 0.f,
		.maxDepth = 1.f
	};
	pVulkanData->functions.vkCmdSetViewport(pVulkanData->pSecondaryCommandBuffers[commandBufferIndex], 0, 1, &viewport);

	const VkRect2D scissor = {
		.offset = {0, 0},
		.extent = pVulkanData->extent
	};
	pVulkanData->functions.vkCmdSetScissor(pVulkanData->pSecondaryCommandBuffers[commandBufferIndex], 0, 1, &scissor);

	static const VkDeviceSize pOffsets[] = {0};
	for(uint32_t objectIndex = firstIndex; objectIndex < firstIndex + objectCount; ++objectIndex)
	{
		const FrVulkanObject* pObject = &pVulkanData->objects.pData[objectIndex];

		pVulkanData->functions.vkCmdBindPipeline(pVulkanData->pSecondaryCommandBuffers[commandBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pVulkanData->graphicsPipelines.pData[pObject->pipelineIndex].pipeline);

		pVulkanData->functions.vkCmdBindVertexBuffers(pVulkanData->pSecondaryCommandBuffers[commandBufferIndex], 0, 1, &pObject->buffer, pOffsets);
		pVulkanData->functions.vkCmdBindIndexBuffer(pVulkanData->pSecondaryCommandBuffers[commandBufferIndex], pObject->buffer, pObject->vertexCount * sizeof(FrVertex), VK_INDEX_TYPE_UINT32);

		pVulkanData->functions.vkCmdBindDescriptorSets(pVulkanData->pSecondaryCommandBuffers[commandBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pVulkanData->graphicsPipelines.pData[pObject->pipelineIndex].pipelineLayout, 0, 1, &pObject->descriptorSets[pVulkanData->frameInFlightIndex], 0, NULL);
		pVulkanData->functions.vkCmdPushConstants(pVulkanData->pSecondaryCommandBuffers[commandBufferIndex], pVulkanData->graphicsPipelines.pData[pObject->pipelineIndex].pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pObject->transformation), pObject->transformation);

		pVulkanData->functions.vkCmdDrawIndexed(pVulkanData->pSecondaryCommandBuffers[commandBufferIndex], pObject->indexCount, 1, 0, 0, 0);
	}

	pVulkanData->functions.vkEndCommandBuffer(pVulkanData->pSecondaryCommandBuffers[commandBufferIndex]);

	return FR_SUCCESS;
}

static int frThreadFunction(FrThreadData* pThreadData)
{
	while(!pThreadData->pVulkanData->threadShouldExit)
	{
		frLockMutex(&pThreadData->pVulkanData->frameBeginMutex);
		do
		{
			if(frWaitConditionVariable(&pThreadData->pVulkanData->frameBeginConditionVariable, &pThreadData->pVulkanData->frameBeginMutex) != FR_SUCCESS) return EXIT_FAILURE;
		} while(!pThreadData->canRun);
		frUnlockMutex(&pThreadData->pVulkanData->frameBeginMutex);

		pThreadData->canRun = false;

		if(frRecordCommandBuffer(pThreadData->pVulkanData, pThreadData->threadIndex) != FR_SUCCESS) return EXIT_FAILURE;

		if(frLockMutex(&pThreadData->pVulkanData->frameEndMutex) != FR_SUCCESS) return EXIT_FAILURE;

		if(++pThreadData->pVulkanData->frameFinishedThreadCount == pThreadData->pVulkanData->threadCount)
		{
			if(frSignalConditionVariable(&pThreadData->pVulkanData->frameEndConditionVariable) != FR_SUCCESS) return EXIT_FAILURE;
		}

		if(frUnlockMutex(&pThreadData->pVulkanData->frameEndMutex) != FR_SUCCESS) return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

FrResult frCreateCommandPools(FrVulkanData* pVulkanData)
{
	pVulkanData->frameInFlightIndex = 0;

	if(frInitializeMutex(&pVulkanData->frameBeginMutex) != FR_SUCCESS) return FR_ERROR_UNKNOWN;
	if(frInitializeConditionVariable(&pVulkanData->frameBeginConditionVariable) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	if(frInitializeMutex(&pVulkanData->frameEndMutex) != FR_SUCCESS) return FR_ERROR_UNKNOWN;
	if(frInitializeConditionVariable(&pVulkanData->frameEndConditionVariable) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	pVulkanData->threadShouldExit = false;

	pVulkanData->threadCount = frGetNumberOfLogicalCores();
	pVulkanData->frameFinishedThreadCount = pVulkanData->threadCount;
	pVulkanData->pThreads = malloc((pVulkanData->threadCount - 1) * sizeof(FrThread));
	if(!pVulkanData->pThreads) return FR_ERROR_OUT_OF_MEMORY;

	pVulkanData->pThreadsData = malloc((pVulkanData->threadCount - 1) * sizeof(FrThreadData));
	if(!pVulkanData->pThreadsData)
	{
		free(pVulkanData->pThreads);
		return FR_ERROR_OUT_OF_MEMORY;
	}

	for(uint32_t threadIndex = 0; threadIndex < pVulkanData->threadCount - 1; ++threadIndex)
	{
		pVulkanData->pThreadsData[threadIndex] = (FrThreadData){
			.pVulkanData = pVulkanData,
			.threadIndex = threadIndex + 1,
			.canRun = false
		};

		if(frCreateThread(&pVulkanData->pThreads[threadIndex], (FrThreadProc)frThreadFunction, &pVulkanData->pThreadsData[threadIndex]) != FR_SUCCESS)
		{
			free(pVulkanData->pThreadsData);
			free(pVulkanData->pThreads);
			return FR_ERROR_UNKNOWN;
		}
	}

	pVulkanData->pCommandPools = malloc(pVulkanData->threadCount * FR_FRAMES_IN_FLIGHT * sizeof(VkCommandPool));
	if(!pVulkanData->pCommandPools)
	{
		free(pVulkanData->pThreadsData);
		free(pVulkanData->pThreads);
		return FR_ERROR_OUT_OF_MEMORY;
	}

	pVulkanData->pSecondaryCommandBuffers = malloc(pVulkanData->threadCount * FR_FRAMES_IN_FLIGHT * sizeof(VkCommandBuffer));
	if(!pVulkanData->pSecondaryCommandBuffers)
	{
		free(pVulkanData->pCommandPools);
		free(pVulkanData->pThreadsData);
		free(pVulkanData->pThreads);
		return FR_ERROR_OUT_OF_MEMORY;
	}

	for(uint32_t i = 0; i < pVulkanData->threadCount * FR_FRAMES_IN_FLIGHT; ++i)
	{
		VkCommandPoolCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = pVulkanData->queueFamily
		};
		if(pVulkanData->functions.vkCreateCommandPool(pVulkanData->device, &createInfo, NULL, &pVulkanData->pCommandPools[i]) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

#ifndef NDEBUG
		if(pVulkanData->debugExtensionAvailable)
		{
			char name[64];
			if(snprintf(name, sizeof(name), "Command pool thread %u frame %u", i % pVulkanData->threadCount, i / pVulkanData->threadCount) < 0) return FR_ERROR_UNKNOWN;
			VkDebugUtilsObjectNameInfoEXT nameInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_COMMAND_POOL,
				.objectHandle = (uint64_t)pVulkanData->pCommandPools[i],
				.pObjectName = name
			};
			if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
		}
#endif

		if(i % pVulkanData->threadCount == 0)
		{
			VkCommandBufferAllocateInfo allocateInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = pVulkanData->pCommandPools[i],
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = 1
			};
			if(pVulkanData->functions.vkAllocateCommandBuffers(pVulkanData->device, &allocateInfo, &pVulkanData->pPrimaryCommandBuffers[i / pVulkanData->threadCount]) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

#ifndef NDEBUG
			if(pVulkanData->debugExtensionAvailable)
			{
				char name[64];
				if(snprintf(name, sizeof(name), "Primary command buffer %u", i / pVulkanData->threadCount) < 0) return FR_ERROR_UNKNOWN;
				VkDebugUtilsObjectNameInfoEXT nameInfo = {
					.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
					.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
					.objectHandle = (uint64_t)pVulkanData->pPrimaryCommandBuffers[i / pVulkanData->threadCount],
					.pObjectName = name
				};
				if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
			}
#endif
		}

		VkCommandBufferAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = pVulkanData->pCommandPools[i],
			.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			.commandBufferCount = 1
		};
		if(pVulkanData->functions.vkAllocateCommandBuffers(pVulkanData->device, &allocateInfo, &pVulkanData->pSecondaryCommandBuffers[i]) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

#ifndef NDEBUG
		if(pVulkanData->debugExtensionAvailable)
		{
			char name[64];
			if(snprintf(name, sizeof(name), "Secondary command buffer %u frame %u", i % pVulkanData->threadCount, i / pVulkanData->threadCount) < 0) return FR_ERROR_UNKNOWN;
			VkDebugUtilsObjectNameInfoEXT nameInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
				.objectHandle = (uint64_t)pVulkanData->pSecondaryCommandBuffers[i],
				.pObjectName = name
			};
			if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
		}
#endif
	}

	for(uint32_t i = 0; i < FR_FRAMES_IN_FLIGHT; ++i)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		VkFenceCreateInfo fenceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};
		if(pVulkanData->functions.vkCreateSemaphore(pVulkanData->device, &semaphoreCreateInfo, NULL, &pVulkanData->imageAvailableSemaphores[i]) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
		if(pVulkanData->functions.vkCreateSemaphore(pVulkanData->device, &semaphoreCreateInfo, NULL, &pVulkanData->renderFinishedSemaphores[i]) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
		if(pVulkanData->functions.vkCreateFence(pVulkanData->device, &fenceCreateInfo, NULL, &pVulkanData->frameInFlightFences[i]) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

#ifndef NDEBUG
		if(pVulkanData->debugExtensionAvailable)
		{
			char name[64];
			if(snprintf(name, sizeof(name), "Image available semaphore %u", i) < 0) return FR_ERROR_UNKNOWN;
			VkDebugUtilsObjectNameInfoEXT nameInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_SEMAPHORE,
				.objectHandle = (uint64_t)pVulkanData->imageAvailableSemaphores[i],
				.pObjectName = name
			};
			if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

			if(snprintf(name, sizeof(name), "Render finished semaphore %u", i) < 0) return FR_ERROR_UNKNOWN;
			nameInfo.objectHandle = (uint64_t)pVulkanData->renderFinishedSemaphores[i];
			if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

			if(snprintf(name, sizeof(name), "Frame in flight fence %u", i) < 0) return FR_ERROR_UNKNOWN;
			nameInfo.objectType = VK_OBJECT_TYPE_FENCE;
			nameInfo.objectHandle = (uint64_t)pVulkanData->frameInFlightFences[i];
			if(pVulkanData->functions.vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
		}
#endif
	}

	return FR_SUCCESS;
}

FrResult frDrawFrame(FrVulkanData* pVulkanData)
{
	static struct timespec lastTime = {0,0}, currentTime;

	if(pVulkanData->functions.vkWaitForFences(pVulkanData->device, 1, &pVulkanData->frameInFlightFences[pVulkanData->frameInFlightIndex], VK_TRUE, UINT64_MAX) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	VkResult result = pVulkanData->functions.vkAcquireNextImageKHR(pVulkanData->device, pVulkanData->swapchain, UINT64_MAX, pVulkanData->imageAvailableSemaphores[pVulkanData->frameInFlightIndex], VK_NULL_HANDLE, &pVulkanData->imageIndex);
	if(result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		pVulkanData->pApplication->window.resized = true;
		return FR_SUCCESS;
	}
	else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		return FR_ERROR_UNKNOWN;
	}

	if(pVulkanData->functions.vkResetFences(pVulkanData->device, 1, &pVulkanData->frameInFlightFences[pVulkanData->frameInFlightIndex]) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	for(uint32_t commandPoolIndex = pVulkanData->frameInFlightIndex * pVulkanData->threadCount; commandPoolIndex < (pVulkanData->frameInFlightIndex + 1) * pVulkanData->threadCount; ++commandPoolIndex)
	{
		if(pVulkanData->functions.vkResetCommandPool(pVulkanData->device, pVulkanData->pCommandPools[commandPoolIndex], 0) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}

	// Begin command buffer and render pass
	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	if(pVulkanData->functions.vkBeginCommandBuffer(pVulkanData->pPrimaryCommandBuffers[pVulkanData->frameInFlightIndex], &beginInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	VkClearValue clearColor = {
		.color.float32 = {0.f, 0.f, 0.f, 0.f}
	};
	VkClearValue clearDepthStencil = {
		.depthStencil = {1.f, 0}
	};
	VkClearValue clearValues[] = {
		clearColor,
		clearDepthStencil
	};
	VkRenderPassBeginInfo renderPassBegin = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = pVulkanData->renderPass,
		.framebuffer = pVulkanData->pFramebuffers[pVulkanData->imageIndex],
		.renderArea.offset = {0, 0},
		.renderArea.extent = pVulkanData->extent,
		.clearValueCount = sizeof(clearValues) / sizeof(VkClearValue),
		.pClearValues = clearValues
	};
	pVulkanData->functions.vkCmdBeginRenderPass(pVulkanData->pPrimaryCommandBuffers[pVulkanData->frameInFlightIndex], &renderPassBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	// Update application state
	if(timespec_get(&currentTime, TIME_UTC) != TIME_UTC) return FR_ERROR_UNKNOWN;
	pVulkanData->updateHandler(
		pVulkanData,
		lastTime.tv_sec == 0 && lastTime.tv_nsec == 0 ? 0 :
			(float)(((uintmax_t)currentTime.tv_sec - (uintmax_t)lastTime.tv_sec) * UINTMAX_C(1000000000) + (uintmax_t)currentTime.tv_nsec - (uintmax_t)lastTime.tv_nsec) / 1000000000.f,
		pVulkanData->pUpdateHandlerUserData
	);
	lastTime = currentTime;

	// Update camera
	static float cameraMatrix[16];
	frGetCameraMatrix(&pVulkanData->pApplication->camera, cameraMatrix);
	memcpy(pVulkanData->uniformBuffers.pData[0].pBufferDatas[pVulkanData->frameInFlightIndex], cameraMatrix, sizeof(cameraMatrix));

	// Record command buffers to draw objects
	pVulkanData->frameFinishedThreadCount = 0;
	for(uint32_t threadIndex = 0; threadIndex < pVulkanData->threadCount; ++threadIndex)
	{
		pVulkanData->pThreadsData[threadIndex].canRun = true;
	}

	if(frBroadcastConditionVariable(&pVulkanData->frameBeginConditionVariable) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	if(frRecordCommandBuffer(pVulkanData, 0) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	if(frLockMutex(&pVulkanData->frameEndMutex) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	if(++pVulkanData->frameFinishedThreadCount < pVulkanData->threadCount)
	{
		do
		{
			if(frWaitConditionVariable(&pVulkanData->frameEndConditionVariable, &pVulkanData->frameEndMutex) != FR_SUCCESS) return FR_ERROR_UNKNOWN;
		} while(pVulkanData->frameFinishedThreadCount < pVulkanData->threadCount);
	}

	if(frUnlockMutex(&pVulkanData->frameEndMutex) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	pVulkanData->functions.vkCmdExecuteCommands(
		pVulkanData->pPrimaryCommandBuffers[pVulkanData->frameInFlightIndex],
		pVulkanData->threadCount,
		pVulkanData->pSecondaryCommandBuffers + pVulkanData->frameInFlightIndex * pVulkanData->threadCount
	);

	// End render pass and command buffer
	pVulkanData->functions.vkCmdEndRenderPass(pVulkanData->pPrimaryCommandBuffers[pVulkanData->frameInFlightIndex]);

	if(pVulkanData->functions.vkEndCommandBuffer(pVulkanData->pPrimaryCommandBuffers[pVulkanData->frameInFlightIndex]) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	static const VkPipelineStageFlags pWaitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &pVulkanData->imageAvailableSemaphores[pVulkanData->frameInFlightIndex],
		.pWaitDstStageMask = pWaitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &pVulkanData->pPrimaryCommandBuffers[pVulkanData->frameInFlightIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &pVulkanData->renderFinishedSemaphores[pVulkanData->frameInFlightIndex]
	};
	if(pVulkanData->functions.vkQueueSubmit(pVulkanData->queue, 1, &submitInfo, pVulkanData->frameInFlightFences[pVulkanData->frameInFlightIndex]) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	VkResult presentResult;
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &pVulkanData->renderFinishedSemaphores[pVulkanData->frameInFlightIndex],
		.swapchainCount = 1,
		.pSwapchains = &pVulkanData->swapchain,
		.pImageIndices = &pVulkanData->imageIndex,
		.pResults = &presentResult
	};
	result = pVulkanData->functions.vkQueuePresentKHR(pVulkanData->queue, &presentInfo);
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		pVulkanData->pApplication->window.resized = true;
		return FR_SUCCESS;
	}
	else if(result != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	pVulkanData->frameInFlightIndex = (pVulkanData->frameInFlightIndex + 1) % FR_FRAMES_IN_FLIGHT;

	return FR_SUCCESS;
}

FrResult frRecreateSwapchain(FrVulkanData* pVulkanData)
{
	if(pVulkanData->functions.vkDeviceWaitIdle(pVulkanData->device) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	VkSwapchainKHR oldSwapchain = pVulkanData->swapchain;

	for(uint32_t imageIndex = 0; imageIndex < pVulkanData->imageCount; ++imageIndex)
	{
		pVulkanData->functions.vkDestroyFramebuffer(pVulkanData->device, pVulkanData->pFramebuffers[imageIndex], NULL);
		pVulkanData->functions.vkDestroyImageView(pVulkanData->device, pVulkanData->pImageViews[imageIndex], NULL);
	}
	pVulkanData->functions.vkDestroyImageView(pVulkanData->device, pVulkanData->depthImageView, NULL);
	pVulkanData->functions.vkDestroyImage(pVulkanData->device, pVulkanData->depthImage, NULL);
	pVulkanData->functions.vkFreeMemory(pVulkanData->device, pVulkanData->depthImageMemory, NULL);
	pVulkanData->functions.vkDestroyImageView(pVulkanData->device, pVulkanData->colorImageView, NULL);
	pVulkanData->functions.vkDestroyImage(pVulkanData->device, pVulkanData->colorImage, NULL);
	pVulkanData->functions.vkFreeMemory(pVulkanData->device, pVulkanData->colorImageMemory, NULL);

	if(frCreateSwapchain(pVulkanData) != FR_SUCCESS) return FR_ERROR_UNKNOWN;
	if(frCreateColorImage(pVulkanData) != FR_SUCCESS) return FR_ERROR_UNKNOWN;
	if(frCreateDepthImage(pVulkanData) != FR_SUCCESS) return FR_ERROR_UNKNOWN;
	if(frCreateFramebuffers(pVulkanData) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	pVulkanData->functions.vkDestroySwapchainKHR(pVulkanData->device, oldSwapchain, NULL);

	pVulkanData->pApplication->window.resized = false;

	return FR_SUCCESS;
}

void frSetUpdateHandler(FrVulkanData* pVulkanData, FrUpdateHandler handler, void* pUserData)
{
	pVulkanData->updateHandler = handler;
	pVulkanData->pUpdateHandlerUserData = pUserData;
}
