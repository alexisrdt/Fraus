#include "instance.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "functions.h"

static bool frLayersAvailable(VkLayerProperties* pAvailableLayers, uint32_t availableLayerCount, const char** ppLayers, uint32_t layerCount)
{
	for(uint32_t i = 0; i < layerCount; ++i)
	{
		uint32_t j;
		for(j = 0; j < availableLayerCount; ++j)
		{
			if(strcmp(ppLayers[i], pAvailableLayers[j].layerName) == 0) break;
		}

		if(j == availableLayerCount) return false;
	}

	return true;
}

static bool frExtensionsAvailable(VkExtensionProperties* pAvailableExtensions, uint32_t availableExtensionCount, const char** ppExtensions, uint32_t extensionCount)
{
	for(uint32_t i = 0; i < extensionCount; ++i)
	{
		uint32_t j;
		for(j = 0; j < availableExtensionCount; ++j)
		{
			if(strcmp(ppExtensions[i], pAvailableExtensions[j].extensionName) == 0) break;
		}

		if(j == availableExtensionCount) return false;
	}

	return true;
}

#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL frMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
#ifdef _WIN32
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

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
	}

	// Message
	printf("] %s\n", pCallbackData->pMessage);

#ifdef _WIN32
	// Reset console color
	SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#endif

	return VK_FALSE;
}
#endif

FrResult frCreateInstance(const char* pName, uint32_t version, FrVulkanData* pVulkanData)
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
	const uint32_t layerCount = sizeof(ppLayers) / sizeof(const char*);

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
		VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifndef NDEBUG
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
	};
	const uint32_t extensionCount = sizeof(ppExtensions) / sizeof(const char*);

#ifndef NDEBUG
	if(!frExtensionsAvailable(pAvailableExtensions, availableExtensionCount, ppExtensions, extensionCount - 1))
	{
		free(pAvailableExtensions);
		return FR_ERROR_UNKNOWN;
	}

	pVulkanData->debugExtensionAvailable = frExtensionsAvailable(pAvailableExtensions, availableExtensionCount, ppExtensions + extensionCount - 1, 1);
	free(pAvailableExtensions);

	if(!pVulkanData->debugExtensionAvailable && validationLayerAvailable)
	{
		uint32_t availableExtensionCount;
		if(vkEnumerateInstanceExtensionProperties("VK_LAYER_KHRONOS_validation", &availableExtensionCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
		VkExtensionProperties* pAvailableExtensions = malloc(availableExtensionCount * sizeof(VkExtensionProperties));
		if(!pAvailableExtensions) return FR_ERROR_OUT_OF_MEMORY;
		if(vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, pAvailableExtensions) != VK_SUCCESS)
		{
			free(pAvailableExtensions);
			return FR_ERROR_UNKNOWN;
		}

		pVulkanData->debugExtensionAvailable = frExtensionsAvailable(pAvailableExtensions, availableExtensionCount, ppExtensions + extensionCount - 1, 1);
		free(pAvailableExtensions);
	}

	if(!pVulkanData->debugExtensionAvailable) printf("Debug extension not found.\n");
#else
	if(!frExtensionsAvailable(pAvailableExtensions, availableExtensionCount, ppExtensions, extensionCount))
	{
		free(pAvailableExtensions);
		return FR_ERROR_UNKNOWN;
	}
	free(pAvailableExtensions);
#endif

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
		.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 1),
		.apiVersion = VK_API_VERSION_1_0
	};

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &applicationInfo,
#ifndef NDEBUG
		.pNext = pVulkanData->debugExtensionAvailable ? &messengerCreateInfo : NULL,
		.enabledLayerCount = validationLayerAvailable ? layerCount : 0,
		.ppEnabledLayerNames = ppLayers,
		.enabledExtensionCount = pVulkanData->debugExtensionAvailable ? extensionCount : extensionCount - 1,
#else
		.enabledExtensionCount = extensionCount,
#endif
		.ppEnabledExtensionNames = ppExtensions
	};

	if(vkCreateInstance(&createInfo, NULL, &pVulkanData->instance) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkDestroyInstance)
#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkCreateDebugUtilsMessengerEXT)
		FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkDestroyDebugUtilsMessengerEXT)
	}
#endif
#ifdef _WIN32
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkCreateWin32SurfaceKHR)
#endif
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkDestroySurfaceKHR)
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkEnumeratePhysicalDevices)
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkGetPhysicalDeviceSurfaceFormatsKHR)
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkGetPhysicalDeviceSurfacePresentModesKHR)
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkGetDeviceProcAddr)
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkEnumerateDeviceLayerProperties)
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkEnumerateDeviceExtensionProperties)
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkCreateDevice)

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		messengerCreateInfo.pUserData = &pVulkanData->instance;
		if(vkCreateDebugUtilsMessengerEXT(pVulkanData->instance, &messengerCreateInfo, NULL, &pVulkanData->messenger) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

FrResult frChoosePhysicalDevice(FrVulkanData* pVulkanData)
{
	uint32_t physicalDeviceCount;
	if(vkEnumeratePhysicalDevices(pVulkanData->instance, &physicalDeviceCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	VkPhysicalDevice* pPhysicalDevices = malloc(physicalDeviceCount * sizeof(VkPhysicalDevice));
	if(!pPhysicalDevices) return FR_ERROR_OUT_OF_MEMORY;
	if(vkEnumeratePhysicalDevices(pVulkanData->instance, &physicalDeviceCount, pPhysicalDevices) != VK_SUCCESS)
	{
		free(pPhysicalDevices);
		return FR_ERROR_UNKNOWN;
	}

	pVulkanData->physicalDevice = pPhysicalDevices[0];

	free(pPhysicalDevices);

	return FR_SUCCESS;
}

FrResult frCreateSurface(FrWindow* pWindow, FrVulkanData* pVulkanData)
{
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = GetModuleHandle(NULL),
		.hwnd = pWindow->handle
	};
	if(vkCreateWin32SurfaceKHR(pVulkanData->instance, &createInfo, NULL, &pVulkanData->surface) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
#endif

	pVulkanData->swapchain = VK_NULL_HANDLE;

	return FR_SUCCESS;
}

FrResult frCreateDevice(FrVulkanData* pVulkanData)
{
	float priority = 1.f;

	uint32_t availableExtensionCount;
	if(vkEnumerateDeviceExtensionProperties(pVulkanData->physicalDevice, NULL, &availableExtensionCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	VkExtensionProperties* pAvailableExtensions = malloc(availableExtensionCount * sizeof(VkExtensionProperties));
	if(!pAvailableExtensions) return FR_ERROR_UNKNOWN;
	if(vkEnumerateDeviceExtensionProperties(pVulkanData->physicalDevice, NULL, &availableExtensionCount, pAvailableExtensions) != VK_SUCCESS)
	{
		free(pAvailableExtensions);
		return FR_ERROR_UNKNOWN;
	}

	const char* ppExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	const uint32_t extensionCount = sizeof(ppExtensions) / sizeof(const char*);

	if(!frExtensionsAvailable(pAvailableExtensions, availableExtensionCount, ppExtensions, extensionCount))
	{
		free(pAvailableExtensions);
		return FR_ERROR_UNKNOWN;
	}

	free(pAvailableExtensions);

	VkDeviceQueueCreateInfo queueCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = 0,
		.queueCount = 1,
		.pQueuePriorities = &priority
	};

	VkDeviceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = extensionCount,
		.ppEnabledExtensionNames = ppExtensions
	};

// Load device layers for compatibility
#ifndef NDEBUG
	uint32_t availableLayerCount;
	if(vkEnumerateDeviceLayerProperties(pVulkanData->physicalDevice, &availableLayerCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	VkLayerProperties* pAvailableLayers = malloc(availableLayerCount * sizeof(VkLayerProperties));
	if(!pAvailableLayers) return FR_ERROR_OUT_OF_MEMORY;
	if(vkEnumerateDeviceLayerProperties(pVulkanData->physicalDevice, &availableLayerCount, pAvailableLayers) != VK_SUCCESS)
	{
		free(pAvailableLayers);
		return FR_ERROR_UNKNOWN;
	}

	const char* ppLayers[] = {"VK_LAYER_KHRONOS_validation"};
	const uint32_t layerCount = sizeof(ppLayers) / sizeof(const char*);

	if(frLayersAvailable(pAvailableLayers, availableLayerCount, ppLayers, layerCount))
	{
		createInfo.enabledLayerCount = layerCount;
		createInfo.ppEnabledLayerNames = ppLayers;
	}

	free(pAvailableLayers);
#endif

	if(vkCreateDevice(pVulkanData->physicalDevice, &createInfo, NULL, &pVulkanData->device) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkDestroyDevice)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkCreateSwapchainKHR)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkDestroySwapchainKHR)

	return FR_SUCCESS;
}

FrResult frCreateSwapchain(FrVulkanData* pVulkanData)
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pVulkanData->physicalDevice, pVulkanData->surface, &surfaceCapabilities) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	uint32_t surfaceFormatCount;
	if(vkGetPhysicalDeviceSurfaceFormatsKHR(pVulkanData->physicalDevice, pVulkanData->surface, &surfaceFormatCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	VkSurfaceFormatKHR* pSurfaceFormats = malloc(surfaceFormatCount * sizeof(VkSurfaceFormatKHR));
	if(!pSurfaceFormats) return FR_ERROR_OUT_OF_MEMORY;
	if(vkGetPhysicalDeviceSurfaceFormatsKHR(pVulkanData->physicalDevice, pVulkanData->surface, &surfaceFormatCount, pSurfaceFormats) != VK_SUCCESS)
	{
		free(pSurfaceFormats);
		return FR_ERROR_UNKNOWN;
	}
	uint32_t surfaceFormatIndex = 0;
	for(uint32_t i = 0; i < surfaceFormatCount; ++i)
	{
		if(pSurfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
		{
			surfaceFormatIndex = i;
			break;
		}
	}

	uint32_t presentModeCount;
	if(vkGetPhysicalDeviceSurfacePresentModesKHR(pVulkanData->physicalDevice, pVulkanData->surface, &presentModeCount, NULL) != VK_SUCCESS)
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
	if(vkGetPhysicalDeviceSurfacePresentModesKHR(pVulkanData->physicalDevice, pVulkanData->surface, &presentModeCount, pPresentModes) != VK_SUCCESS)
	{
		free(pSurfaceFormats);
		free(pPresentModes);
		return FR_ERROR_UNKNOWN;
	}

	uint32_t presentModeIndex = 0;
	for(uint32_t i = 0; i < presentModeCount; ++i)
	{
		if(pPresentModes[i] == VK_PRESENT_MODE_FIFO_KHR)
		{
			presentModeIndex = i;
			break;
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
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.preTransform = surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = pPresentModes[presentModeIndex],
		.clipped = VK_TRUE,
		.oldSwapchain = pVulkanData->swapchain
	};

	if(vkCreateSwapchainKHR(pVulkanData->device, &createInfo, NULL, &pVulkanData->swapchain) != VK_SUCCESS)
	{
		free(pSurfaceFormats);
		free(pPresentModes);
		return EXIT_FAILURE;
	}

	free(pSurfaceFormats);
	free(pPresentModes);

	return FR_SUCCESS;
}
