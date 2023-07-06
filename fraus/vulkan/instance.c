#include "instance.h"

#include <stdbool.h>
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

FrResult frCreateInstance(const char* pName, uint32_t version, FrVulkanData* pVulkanData)
{
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
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
	};
	const uint32_t extensionCount = sizeof(ppExtensions) / sizeof(const char*);

	if(!frExtensionsAvailable(pAvailableExtensions, availableExtensionCount, ppExtensions, extensionCount))
	{
		free(pAvailableExtensions);
		return FR_ERROR_UNKNOWN;
	}

	free(pAvailableExtensions);

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
		.enabledExtensionCount = sizeof(ppExtensions) / sizeof(const char*),
		.ppEnabledExtensionNames = ppExtensions
	};

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

	if(frLayersAvailable(pAvailableLayers, availableLayerCount, ppLayers, layerCount))
	{
		createInfo.enabledLayerCount = layerCount;
		createInfo.ppEnabledLayerNames = ppLayers;
	}

	free(pAvailableLayers);
#endif

	if(vkCreateInstance(&createInfo, NULL, &pVulkanData->instance) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkDestroyInstance)
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
		.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED,
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
