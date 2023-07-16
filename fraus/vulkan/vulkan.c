#include "vulkan.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "functions.h"

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

/*
 * Check that required extensions are available
 * - pAvailableExtensions: available extensions
 * - availableExtensionCount: number of available extensions
 * - ppExtensions: required extensions
 * - extensionCount: number of required extensions
 */
static bool frExtensionsAvailable(VkExtensionProperties* pAvailableExtensions, uint32_t availableExtensionCount, const char** ppExtensions, uint32_t extensionCount)
{
	uint32_t availableExtensionIndex;
	for(uint32_t extensionIndex = 0; extensionIndex < extensionCount; ++extensionIndex)
	{
		for(availableExtensionIndex = 0; availableExtensionIndex < availableExtensionCount; ++availableExtensionIndex)
		{
			if(strcmp(ppExtensions[extensionIndex], pAvailableExtensions[availableExtensionIndex].extensionName) == 0) break;
		}

		if(availableExtensionIndex == availableExtensionCount) return false;
	}

	return true;
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

	// Object names
	for(uint32_t i = 0; i < pCallbackData->objectCount; ++i) printf("|%s", pCallbackData->pObjects[i].pObjectName);

	// Message
	printf("] %s\n", pCallbackData->pMessage);

#ifdef _WIN32
	// Reset console color
	SetConsoleTextAttribute(console, consoleInfo.wAttributes);
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
		FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkSetDebugUtilsObjectNameEXT)
	}
#endif
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkGetPhysicalDeviceSurfaceSupportKHR)
#ifdef _WIN32
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkCreateWin32SurfaceKHR)
#endif
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkDestroySurfaceKHR)
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkEnumeratePhysicalDevices)
	FR_LOAD_INSTANCE_PFN(pVulkanData->instance, vkGetPhysicalDeviceQueueFamilyProperties)
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

	for(uint32_t i = 0; i < physicalDeviceCount; ++i)
	{
		uint32_t queueFamilyPropertyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevices[i], &queueFamilyPropertyCount, NULL);
		VkQueueFamilyProperties* pQueueFamilyProperties = malloc(queueFamilyPropertyCount * sizeof(VkQueueFamilyProperties));
		if(!pQueueFamilyProperties)
		{
			free(pPhysicalDevices);
			return FR_ERROR_UNKNOWN;
		}
		vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevices[i], &queueFamilyPropertyCount, pQueueFamilyProperties);

		for(uint32_t j = 0; j < queueFamilyPropertyCount; ++j)
		{
			VkBool32 surfaceSupported;
			if(vkGetPhysicalDeviceSurfaceSupportKHR(pPhysicalDevices[i], j, pVulkanData->surface, &surfaceSupported) != VK_SUCCESS)
			{
				free(pQueueFamilyProperties);
				free(pPhysicalDevices);
				return FR_ERROR_UNKNOWN;
			}

			if(pQueueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT && surfaceSupported)
			{
				pVulkanData->physicalDevice = pPhysicalDevices[i];
				pVulkanData->queueFamily = j;

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
		.queueFamilyIndex = pVulkanData->queueFamily,
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
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkGetDeviceQueue)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkCreateSwapchainKHR)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkDestroySwapchainKHR)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkGetSwapchainImagesKHR)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkCreateImageView)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkDestroyImageView)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkCreateFramebuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkDestroyFramebuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkCreateRenderPass)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkDestroyRenderPass)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkCreateSemaphore)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkDestroySemaphore)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkCreateFence)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkDestroyFence)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkWaitForFences)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkResetFences)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkCreateCommandPool)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkDestroyCommandPool)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkAllocateCommandBuffers)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkResetCommandBuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkAcquireNextImageKHR)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkQueuePresentKHR)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkQueueSubmit)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkBeginCommandBuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkEndCommandBuffer)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkCmdBeginRenderPass)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkCmdEndRenderPass)
	FR_LOAD_DEVICE_PFN(pVulkanData->device, vkDeviceWaitIdle)

	vkGetDeviceQueue(pVulkanData->device, pVulkanData->queueFamily, 0, &pVulkanData->queue);

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_INSTANCE,
			.objectHandle = (uint64_t)pVulkanData->instance,
			.pObjectName = "Fraus instance"
		};
		if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectType = VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT;
		nameInfo.objectHandle = (uint64_t)pVulkanData->messenger;
		nameInfo.pObjectName = "Fraus messenger";
		if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectType = VK_OBJECT_TYPE_PHYSICAL_DEVICE;
		nameInfo.objectHandle = (uint64_t)pVulkanData->physicalDevice;
		nameInfo.pObjectName = "Fraus physical device";
		if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectType = VK_OBJECT_TYPE_SURFACE_KHR;
		nameInfo.objectHandle = (uint64_t)pVulkanData->surface;
		nameInfo.pObjectName = "Fraus surface";
		if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectType = VK_OBJECT_TYPE_DEVICE;
		nameInfo.objectHandle = (uint64_t)pVulkanData->device;
		nameInfo.pObjectName = "Fraus device";
		if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectType = VK_OBJECT_TYPE_QUEUE;
		nameInfo.objectHandle = (uint64_t)pVulkanData->queue;
		nameInfo.pObjectName = "Fraus queue";
		if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

FrResult frCreateSwapchain(FrVulkanData* pVulkanData)
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pVulkanData->physicalDevice, pVulkanData->surface, &surfaceCapabilities) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	pVulkanData->extent = surfaceCapabilities.currentExtent;

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
		if(pSurfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && pSurfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			surfaceFormatIndex = i;
			break;
		}
	}
	pVulkanData->format = pSurfaceFormats[surfaceFormatIndex].format;

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
		.queueFamilyIndexCount = 1, // useless with exclusive
		.pQueueFamilyIndices = &pVulkanData->queueFamily, // useless with exclusive
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

	if(vkGetSwapchainImagesKHR(pVulkanData->device, pVulkanData->swapchain, &pVulkanData->imageCount, NULL) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	pVulkanData->pImages = malloc(pVulkanData->imageCount * sizeof(VkImage));
	if(!pVulkanData->pImages) return FR_ERROR_UNKNOWN;
	if(vkGetSwapchainImagesKHR(pVulkanData->device, pVulkanData->swapchain, &pVulkanData->imageCount, pVulkanData->pImages) != VK_SUCCESS)
	{
		free(pVulkanData->pImages);
		return FR_ERROR_UNKNOWN;
	}

	pVulkanData->pImageViews = malloc(pVulkanData->imageCount * sizeof(VkImageView));
	if(!pVulkanData->pImageViews)
	{
		free(pVulkanData->pImages);
		return FR_ERROR_UNKNOWN;
	}
	for(uint32_t imageIndex = 0; imageIndex < pVulkanData->imageCount; ++imageIndex)
	{
		VkImageViewCreateInfo imageViewInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = pVulkanData->pImages[imageIndex],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = pSurfaceFormats[surfaceFormatIndex].format,
			.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount = 1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount = 1
		};
		if(vkCreateImageView(pVulkanData->device, &imageViewInfo, NULL, &pVulkanData->pImageViews[imageIndex]) != VK_SUCCESS)
		{
			for(uint32_t j = 0; j < imageIndex; ++j)
			{
				vkDestroyImageView(pVulkanData->device, pVulkanData->pImageViews[j], NULL);
			}
			free(pVulkanData->pImageViews);
			free(pVulkanData->pImages);
			return FR_ERROR_UNKNOWN;
		}
	}

	free(pSurfaceFormats);
	free(pPresentModes);

	VkSemaphoreCreateInfo semaphoreCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};
	VkFenceCreateInfo fenceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	if(vkCreateSemaphore(pVulkanData->device, &semaphoreCreateInfo, NULL, &pVulkanData->imageAvailableSemaphore) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	if(vkCreateSemaphore(pVulkanData->device, &semaphoreCreateInfo, NULL, &pVulkanData->renderFinishedSemaphore) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	if(vkCreateFence(pVulkanData->device, &fenceCreateInfo, NULL, &pVulkanData->frameInFlightFence) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR,
			.objectHandle = (uint64_t)pVulkanData->swapchain,
			.pObjectName = "Fraus swapchain"
		};
		if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		char pObjectName[64];
		for(uint32_t imageIndex = 0; imageIndex < pVulkanData->imageCount; ++imageIndex)
		{
			sprintf(pObjectName, "Fraus image #%u", imageIndex);
			nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
			nameInfo.objectHandle = (uint64_t)pVulkanData->pImages[imageIndex];
			nameInfo.pObjectName = pObjectName;
			if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

			sprintf(pObjectName, "Fraus image view #%u", imageIndex);
			nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
			nameInfo.objectHandle = (uint64_t)pVulkanData->pImageViews[imageIndex];
			nameInfo.pObjectName = pObjectName;
			if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
		}

		nameInfo.objectType = VK_OBJECT_TYPE_SEMAPHORE;
		nameInfo.objectHandle = (uint64_t)pVulkanData->imageAvailableSemaphore;
		nameInfo.pObjectName = "Fraus image available semaphore";
		if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectHandle = (uint64_t)pVulkanData->renderFinishedSemaphore;
		nameInfo.pObjectName = "Fraus render finished semaphore";
		if (vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

		nameInfo.objectType = VK_OBJECT_TYPE_FENCE;
		nameInfo.objectHandle = (uint64_t)pVulkanData->frameInFlightFence;
		nameInfo.pObjectName = "Fraus frame in flight fence";
		if (vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

FrResult frCreateRenderPass(FrVulkanData* pVulkanData)
{
	VkAttachmentDescription attachment = {
		.format = pVulkanData->format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference attachmentReference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &attachmentReference
	};

	VkRenderPassCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &attachment,
		.subpassCount = 1,
		.pSubpasses = &subpass
	};

	if(vkCreateRenderPass(pVulkanData->device, &createInfo, NULL, &pVulkanData->renderPass) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_RENDER_PASS,
			.objectHandle = (uint64_t)pVulkanData->renderPass,
			.pObjectName = "Fraus render pass"
		};
		if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

FrResult frCreateFramebuffers(FrVulkanData* pVulkanData)
{
	pVulkanData->pFramebuffers = malloc(pVulkanData->imageCount * sizeof(VkFramebuffer));
	if(!pVulkanData->pFramebuffers) return FR_ERROR_UNKNOWN;

	for(uint32_t imageIndex = 0; imageIndex < pVulkanData->imageCount; ++imageIndex)
	{
		VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = pVulkanData->renderPass,
			.attachmentCount = 1,
			.pAttachments = &pVulkanData->pImageViews[imageIndex],
			.width = pVulkanData->extent.width,
			.height = pVulkanData->extent.height,
			.layers = 1
		};
		if(vkCreateFramebuffer(pVulkanData->device, &framebufferInfo, NULL, &pVulkanData->pFramebuffers[imageIndex]) != VK_SUCCESS)
		{
			for(uint32_t j = 0; j < imageIndex; ++j)
			{
				vkDestroyFramebuffer(pVulkanData->device, pVulkanData->pFramebuffers[j], NULL);
			}

			free(pVulkanData->pFramebuffers);
			return FR_ERROR_UNKNOWN;
		}
	}

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_FRAMEBUFFER
		};
		char pObjectName[64];
		for(uint32_t imageIndex = 0; imageIndex < pVulkanData->imageCount; ++imageIndex)
		{
			sprintf(pObjectName, "Fraus framebuffer #%u", imageIndex);
			nameInfo.objectHandle = (uint64_t)pVulkanData->pFramebuffers[imageIndex];
			nameInfo.pObjectName = pObjectName;
			if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
		}
	}
#endif

	return FR_SUCCESS;
}

FrResult frCreateCommandPool(FrVulkanData* pVulkanData)
{
	VkCommandPoolCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = pVulkanData->queueFamily
	};
	if(vkCreateCommandPool(pVulkanData->device, &createInfo, NULL, &pVulkanData->commandPool) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_COMMAND_POOL,
			.objectHandle = (uint64_t)pVulkanData->commandPool,
			.pObjectName = "Fraus command pool"
		};
		if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	VkCommandBufferAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = pVulkanData->commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	if(vkAllocateCommandBuffers(pVulkanData->device, &allocateInfo, &pVulkanData->commandBuffer) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

#ifndef NDEBUG
	if(pVulkanData->debugExtensionAvailable)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
			.objectHandle = (uint64_t)pVulkanData->commandBuffer,
			.pObjectName = "Fraus command buffer"
		};
		if(vkSetDebugUtilsObjectNameEXT(pVulkanData->device, &nameInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

FrResult frDrawFrame(FrVulkanData* pVulkanData)
{
	static uint32_t imageIndex;

	if(vkWaitForFences(pVulkanData->device, 1, &pVulkanData->frameInFlightFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	if(vkAcquireNextImageKHR(pVulkanData->device, pVulkanData->swapchain, UINT64_MAX, pVulkanData->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	if(vkResetFences(pVulkanData->device, 1, &pVulkanData->frameInFlightFence) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	if(vkResetCommandBuffer(pVulkanData->commandBuffer, 0) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};
	if(vkBeginCommandBuffer(pVulkanData->commandBuffer, &beginInfo) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	VkClearValue clearColor = {
		{{0.f, 0.f, 0.f, 0.f}},
	};
	VkRenderPassBeginInfo renderPassBegin = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = pVulkanData->renderPass,
		.framebuffer = pVulkanData->pFramebuffers[imageIndex],
		.renderArea.offset = {0, 0},
		.renderArea.extent = pVulkanData->extent,
		.clearValueCount = 1,
		.pClearValues = &clearColor
	};
	vkCmdBeginRenderPass(pVulkanData->commandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdEndRenderPass(pVulkanData->commandBuffer);

	if(vkEndCommandBuffer(pVulkanData->commandBuffer) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	static const VkPipelineStageFlags pWaitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &pVulkanData->imageAvailableSemaphore,
		.pWaitDstStageMask = pWaitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &pVulkanData->commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &pVulkanData->renderFinishedSemaphore
	};
	if(vkQueueSubmit(pVulkanData->queue, 1, &submitInfo, pVulkanData->frameInFlightFence) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	VkResult result;
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &pVulkanData->renderFinishedSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &pVulkanData->swapchain,
		.pImageIndices = &imageIndex,
		.pResults = &result
	};
	if(vkQueuePresentKHR(pVulkanData->queue, &presentInfo) != VK_SUCCESS || result != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	return FR_SUCCESS;
}