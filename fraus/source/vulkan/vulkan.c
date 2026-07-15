#include "../../include/fraus/vulkan/vulkan.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <spirv-headers/spirv.h>

#include "../../include/fraus/fraus.h"
#include "./spirv.h"

static HMODULE frVulkanLibrary;
static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

static VkResult frCreateInstance(FrEngine* engine, const char* name, uint32_t version);
static VkResult frChoosePhysicalDevice(FrEngine* engine);
static VkResult frCreateSurface(FrApplication* application);
static VkResult frCreateDevice(FrEngine* engine);
static FrResult frCreateSwapchain(FrEngine* engine);
static VkResult frCreateRenderPass(FrEngine* engine);
static FrResult frCreateFramebuffers(FrEngine* engine);
static FrResult frCreateShaderModule(FrEngine* engine, const char* path, VkShaderModule* shaderModule, FrShaderInfo* info);
static VkResult frCreateSampler(FrEngine* engine);
static VkResult frCreateAttachments(FrEngine* engine);
static VkResult frCreateCommandPools(FrEngine* engine);

#define FR_GLOBAL_PFN(name) \
const PFN_##name name = (PFN_##name)vkGetInstanceProcAddr(nullptr, #name)

#define FR_INSTANCE_PFN(name) \
const PFN_##name name = (PFN_##name)vkGetInstanceProcAddr(engine->instance, #name)

#define FR_DEVICE_PFN(name) \
const PFN_##name name = (PFN_##name)engine->vkGetDeviceProcAddr(engine->device, #name)

FrResult frInitializeVulkan(void)
{
	frVulkanLibrary = LoadLibrary(TEXT("vulkan-1"));
	if(!frVulkanLibrary)
	{
		return FR_ERROR_FILE_NOT_FOUND;
	}

	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)(PFN_vkVoidFunction)GetProcAddress(frVulkanLibrary, "vkGetInstanceProcAddr");
	if(!vkGetInstanceProcAddr)
	{
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

FrResult frFinishVulkan(void)
{
	if(!FreeLibrary(frVulkanLibrary))
	{
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

FrResult frCreateVulkanEngine(FrApplication* const application, const char* const name, const uint32_t version)
{
	application->engine = (FrEngine){
		.arena = &application->arena
	};

	if(frCreateInstance(&application->engine, name, version) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	application->engine.vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(application->engine.instance, "vkGetDeviceProcAddr");

	if(application->engine.hasPhysicalDevice2)
	{
		application->engine.vkGetPhysicalDeviceFormatProperties2 = (PFN_vkGetPhysicalDeviceFormatProperties2)vkGetInstanceProcAddr(application->engine.instance, application->engine.instanceVersion >= VK_API_VERSION_1_1 ? "vkGetPhysicalDeviceFormatProperties2" : "vkGetPhysicalDeviceFormatProperties2KHR");
	}
	else
	{
		application->engine.vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties)vkGetInstanceProcAddr(application->engine.instance, "vkGetPhysicalDeviceFormatProperties");
	}

	application->engine.vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(application->engine.instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	application->engine.vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(application->engine.instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
	application->engine.vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(application->engine.instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");

	if(frCreateSurface(application) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(frChoosePhysicalDevice(&application->engine) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(frCreateDevice(&application->engine) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	application->engine.vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCreateSwapchainKHR");
	application->engine.vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkDestroySwapchainKHR");
	application->engine.vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkGetSwapchainImagesKHR");

	application->engine.vkCreateImage = (PFN_vkCreateImage)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCreateImage");
	application->engine.vkDestroyImage = (PFN_vkDestroyImage)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkDestroyImage");

	application->engine.vkCreateImageView = (PFN_vkCreateImageView)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCreateImageView");
	application->engine.vkDestroyImageView = (PFN_vkDestroyImageView)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkDestroyImageView");

	application->engine.vkCreateShaderModule = (PFN_vkCreateShaderModule)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCreateShaderModule");
	application->engine.vkDestroyShaderModule = (PFN_vkDestroyShaderModule)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkDestroyShaderModule");

	application->engine.vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCreateDescriptorSetLayout");
	application->engine.vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCreatePipelineLayout");
	application->engine.vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCreateGraphicsPipelines");

	application->engine.vkCreateSemaphore = (PFN_vkCreateSemaphore)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCreateSemaphore");
	application->engine.vkDestroySemaphore = (PFN_vkDestroySemaphore)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkDestroySemaphore");

	application->engine.vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkAllocateCommandBuffers");
	application->engine.vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkFreeCommandBuffers");

	application->engine.vkCreateBuffer = (PFN_vkCreateBuffer)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCreateBuffer");
	application->engine.vkDestroyBuffer = (PFN_vkDestroyBuffer)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkDestroyBuffer");
	application->engine.vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkGetBufferMemoryRequirements");
	application->engine.vkBindBufferMemory = (PFN_vkBindBufferMemory)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkBindBufferMemory");
	application->engine.vkAllocateMemory = (PFN_vkAllocateMemory)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkAllocateMemory");
	application->engine.vkFreeMemory = (PFN_vkFreeMemory)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkFreeMemory");
	application->engine.vkMapMemory = (PFN_vkMapMemory)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkMapMemory");
	application->engine.vkUnmapMemory = (PFN_vkUnmapMemory)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkUnmapMemory");
	application->engine.vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkGetImageMemoryRequirements");
	application->engine.vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCreateDescriptorPool");
	application->engine.vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkDestroyDescriptorPool");
	application->engine.vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkAllocateDescriptorSets");
	application->engine.vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkUpdateDescriptorSets");

	application->engine.vkWaitForFences = (PFN_vkWaitForFences)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkWaitForFences");
	application->engine.vkResetFences = (PFN_vkResetFences)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkResetFences");
	application->engine.vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkAcquireNextImageKHR");
	application->engine.vkResetCommandPool = (PFN_vkResetCommandPool)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkResetCommandPool");

	application->engine.vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkBeginCommandBuffer");
	application->engine.vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdCopyBuffer");
	application->engine.vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdCopyBufferToImage");
	application->engine.vkCmdBlitImage = (PFN_vkCmdBlitImage)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdBlitImage");
	application->engine.vkBindImageMemory = (PFN_vkBindImageMemory)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkBindImageMemory");
	application->engine.vkCmdBindPipeline = (PFN_vkCmdBindPipeline)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdBindPipeline");
	application->engine.vkCmdSetViewport = (PFN_vkCmdSetViewport)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdSetViewport");
	application->engine.vkCmdSetScissor = (PFN_vkCmdSetScissor)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdSetScissor");
	application->engine.vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdBindVertexBuffers");
	application->engine.vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdBindIndexBuffer");
	application->engine.vkCmdPushConstants = (PFN_vkCmdPushConstants)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdPushConstants");
	application->engine.vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdBindDescriptorSets");
	application->engine.vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdDrawIndexed");
	application->engine.vkEndCommandBuffer = (PFN_vkEndCommandBuffer)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkEndCommandBuffer");

	application->engine.vkQueuePresentKHR = (PFN_vkQueuePresentKHR)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkQueuePresentKHR");

	application->engine.vkQueueWaitIdle = (PFN_vkQueueWaitIdle)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkQueueWaitIdle");
	application->engine.vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkDeviceWaitIdle");

	if(application->engine.hasDynamicRendering)
	{
		application->engine.vkCmdBeginRendering = (PFN_vkCmdBeginRendering)application->engine.vkGetDeviceProcAddr(application->engine.device, application->engine.deviceVersion >= VK_API_VERSION_1_3 ? "vkCmdBeginRendering" : "vkCmdBeginRenderingKHR");
		application->engine.vkCmdEndRendering = (PFN_vkCmdEndRendering)application->engine.vkGetDeviceProcAddr(application->engine.device, application->engine.deviceVersion >= VK_API_VERSION_1_3 ? "vkCmdEndRendering" : "vkCmdEndRenderingKHR");
	}
	else
	{
		application->engine.vkCreateFramebuffer = (PFN_vkCreateFramebuffer)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCreateFramebuffer");
		application->engine.vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkDestroyFramebuffer");

		if(application->engine.hasRenderPass2)
		{
			application->engine.vkCmdBeginRenderPass2 = (PFN_vkCmdBeginRenderPass2)application->engine.vkGetDeviceProcAddr(application->engine.device, application->engine.deviceVersion >= VK_API_VERSION_1_2 ? "vkCmdBeginRenderPass2" : "vkCmdBeginRenderPass2KHR");
			application->engine.vkCmdEndRenderPass2 = (PFN_vkCmdEndRenderPass2)application->engine.vkGetDeviceProcAddr(application->engine.device, application->engine.deviceVersion >= VK_API_VERSION_1_2 ? "vkCmdEndRenderPass2" : "vkCmdEndRenderPass2KHR");
		}
		else
		{
			application->engine.vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdBeginRenderPass");
			application->engine.vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdEndRenderPass");
		}
	}

	if(application->engine.hasSynchronization2)
	{
		application->engine.vkQueueSubmit2 = (PFN_vkQueueSubmit2)application->engine.vkGetDeviceProcAddr(application->engine.device, application->engine.deviceVersion >= VK_API_VERSION_1_3 ? "vkQueueSubmit2" : "vkQueueSubmit2KHR");
		application->engine.vkCmdPipelineBarrier2 = (PFN_vkCmdPipelineBarrier2)application->engine.vkGetDeviceProcAddr(application->engine.device, application->engine.deviceVersion >= VK_API_VERSION_1_3 ? "vkCmdPipelineBarrier2" : "vkCmdPipelineBarrier2KHR");
	}
	else
	{
		application->engine.vkQueueSubmit = (PFN_vkQueueSubmit)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkQueueSubmit");
		application->engine.vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)application->engine.vkGetDeviceProcAddr(application->engine.device, "vkCmdPipelineBarrier");
	}

	if(frCreateSwapchain(&application->engine) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(!application->engine.hasDynamicRendering)
	{
		if(frCreateRenderPass(&application->engine) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}
	if(frCreateAttachments(&application->engine) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(!application->engine.hasDynamicRendering)
	{
		if(frCreateFramebuffers(&application->engine) != FR_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}
	if(frCreateCommandPools(&application->engine) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(frCreateSampler(&application->engine) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

void frDestroyVulkanEngine(FrEngine* const engine)
{
	if(engine->device)
	{
		FR_DEVICE_PFN(vkDestroyCommandPool);
		FR_DEVICE_PFN(vkDestroyFence);

		for(uint32_t i = 0; i < engine->swapchainImageCount; ++i)
		{
			engine->vkDestroySemaphore(engine->device, engine->renderFinishedSemaphores[i], nullptr);
		}
		for(uint32_t i = 0; i < FR_FRAMES_IN_FLIGHT; ++i)
		{
			vkDestroyCommandPool(engine->device, engine->commandPools[i], nullptr);
			engine->vkDestroySemaphore(engine->device, engine->imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(engine->device, engine->frameInFlightFences[i], nullptr);
		}

		FR_DEVICE_PFN(vkDestroySampler);
		vkDestroySampler(engine->device, engine->textureSampler, nullptr);
		
		for(uint32_t textureIndex = 0; textureIndex < engine->textureCount; ++textureIndex)
		{
			engine->vkDestroyImageView(engine->device, engine->textures[textureIndex].imageView, nullptr);
			engine->vkDestroyImage(engine->device, engine->textures[textureIndex].image, nullptr);
			engine->vkFreeMemory(engine->device, engine->textures[textureIndex].imageMemory, nullptr);
		}

		for(uint32_t storageBufferIndex = 0; storageBufferIndex < engine->storageBufferCount; ++storageBufferIndex)
		{
			engine->vkDestroyBuffer(engine->device, engine->storageBuffers[storageBufferIndex].buffer, nullptr);
			engine->vkFreeMemory(engine->device, engine->storageBuffers[storageBufferIndex].bufferMemory, nullptr);
		}

		for(uint32_t uniformBufferIndex = 0; uniformBufferIndex < engine->uniformBufferCount; ++uniformBufferIndex)
		{
			for(uint32_t frameIndex = 0; frameIndex < FR_FRAMES_IN_FLIGHT; ++frameIndex)
			{
				engine->vkDestroyBuffer(engine->device, engine->uniformBuffers[uniformBufferIndex].buffers[frameIndex], nullptr);
				engine->vkFreeMemory(engine->device, engine->uniformBuffers[uniformBufferIndex].bufferMemories[frameIndex], nullptr);
			}
		}

		for(uint32_t objectIndex = 0; objectIndex < engine->objectCount; ++objectIndex)
		{
			frDestroyObject(engine, &engine->objects[objectIndex]);
		}

		FR_DEVICE_PFN(vkDestroyPipeline);
		FR_DEVICE_PFN(vkDestroyPipelineLayout);
		FR_DEVICE_PFN(vkDestroyDescriptorSetLayout);
		for(uint32_t pipelineIndex = 0; pipelineIndex < engine->graphicsPipelineCount; ++pipelineIndex)
		{
			vkDestroyPipeline(engine->device, engine->graphicsPipelines[pipelineIndex].pipeline, nullptr);
			vkDestroyPipelineLayout(engine->device, engine->graphicsPipelines[pipelineIndex].pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(engine->device, engine->graphicsPipelines[pipelineIndex].descriptorSetLayout, nullptr);
			free(engine->graphicsPipelines[pipelineIndex].descriptorTypes);
		}

		if(!engine->hasDynamicRendering)
		{
			FR_DEVICE_PFN(vkDestroyRenderPass);
			vkDestroyRenderPass(engine->device, engine->renderPass, nullptr);
		}

		engine->vkDestroyImageView(engine->device, engine->depthImageView, nullptr);
		engine->vkDestroyImage(engine->device, engine->depthImage, nullptr);
		engine->vkDestroyImageView(engine->device, engine->colorImageView, nullptr);
		engine->vkDestroyImage(engine->device, engine->colorImage, nullptr);
		engine->vkFreeMemory(engine->device, engine->attachmentsMemory, nullptr);
		for(uint32_t i = 0; i < engine->swapchainImageCount; ++i)
		{
			if(!engine->hasDynamicRendering)
			{
				engine->vkDestroyFramebuffer(engine->device, engine->framebuffers[i], nullptr);
			}
			engine->vkDestroyImageView(engine->device, engine->swapchainImageViews[i], nullptr);
		}
		if(!engine->hasDynamicRendering)
		{
			free(engine->framebuffers);
		}
		free(engine->swapchainImageViews);
		free(engine->swapchainImages);
		engine->vkDestroySwapchainKHR(engine->device, engine->swapchain, nullptr);

		FR_DEVICE_PFN(vkDestroyDevice);
		vkDestroyDevice(engine->device, nullptr);
	}

	if(engine->instance)
	{
		FR_INSTANCE_PFN(vkDestroySurfaceKHR);
		vkDestroySurfaceKHR(engine->instance, engine->surface, nullptr);

#ifndef NDEBUG
		if(engine->hasDebugUtils)
		{
			FR_INSTANCE_PFN(vkDestroyDebugUtilsMessengerEXT);
			vkDestroyDebugUtilsMessengerEXT(engine->instance, engine->messenger, nullptr);
		}
#endif
		FR_INSTANCE_PFN(vkDestroyInstance);
		vkDestroyInstance(engine->instance, nullptr);
	}
}

#ifndef NDEBUG
#define FR_VK_LAYER_KHRONOS_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

/*
 * Check if required layers are available.
 *
 * Parameters:
 * - available: Pointer to store whether layers are available.
 * 
 * Returns:
 * - VK_SUCCESS: No error occurred.
 * - VK_ERROR_OUT_OF_HOST_MEMORY: Out of host memory.
 * - VK_ERROR_OUT_OF_DEVICE_MEMORY: Out of device memory.
 * - VK_ERROR_UNKNOWN: Unknown error.
 */
static VkResult frValidationLayersAvailable(FrArena* const arena, bool* const available)
{
	const FrArenaSave save = frArenaSave(arena);

	FR_GLOBAL_PFN(vkEnumerateInstanceLayerProperties);

	// Get available layers
	uint32_t availableLayerCount;
	VkResult result = vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
	if(result != VK_SUCCESS)
	{
		return result;
	}
	if(availableLayerCount == 0)
	{
		*available = false;
		return result;
	}

	VkLayerProperties* const availableLayers = frArenaAllocate(arena, availableLayerCount * sizeof(availableLayers[0]), alignof(typeof(availableLayers[0])));
	if(!availableLayers)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}
	result = vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers);
	if(result != VK_SUCCESS)
	{
		frArenaRestore(arena, save);
		return result;
	}

	*available = false;
	for(uint32_t availableLayerIndex = 0; availableLayerIndex < availableLayerCount; ++availableLayerIndex)
	{
		if(strcmp(availableLayers[availableLayerIndex].layerName, FR_VK_LAYER_KHRONOS_VALIDATION_LAYER_NAME) == 0)
		{
			*available = true;
			break;
		}
	}

	frArenaRestore(arena, save);

	return result;
}
#endif

/*
 * Check if an extension is available.
 *
 * Parameters:
 * - extension: Extension to search for.
 * - extensions: Available extensions.
 * - extensionCount: Number of available extensions.
 *
 * Returns:
 * - true if the extension is available.
 * - false otherwise.
 */
static bool frExtensionAvailable(const char* const extension, const VkExtensionProperties* const extensions, const uint32_t extensionCount)
{
	for(uint32_t extensionIndex = 0; extensionIndex < extensionCount; ++extensionIndex)
	{
		if(strcmp(extension, extensions[extensionIndex].extensionName) == 0)
		{
			return true;
		}
	}

	return false;
}

#ifndef NDEBUG
/*
 * Debug messenger callback
 * - messageSeverity: message severity
 * - messageTypes: message types
 * - callbackData: message data
 * - userData: user data
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL frMessengerCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, const VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* const callbackData, void* const userData)
{
	(void)userData;

	HANDLE console;
	if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		console = GetStdHandle(STD_ERROR_HANDLE);
	}
	else
	{
		console = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	GetConsoleScreenBufferInfo(console, &consoleInfo);

	// Change console color to identify severity.
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

	fputc('[', stdout);

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

	// Message
	printf("] %s\n", callbackData->pMessage);

	// Reset console color
	SetConsoleTextAttribute(console, consoleInfo.wAttributes);

	return VK_FALSE;
}
#endif

static VkResult frCreateInstance(FrEngine* const engine, const char* const name, const uint32_t version)
{
	const FrArenaSave save = frArenaSave(engine->arena);

	VkResult result;

	engine->instanceVersion = VK_API_VERSION_1_0;
	FR_GLOBAL_PFN(vkEnumerateInstanceVersion);
	if(vkEnumerateInstanceVersion)
	{
		result = vkEnumerateInstanceVersion(&engine->instanceVersion);
		if(result != VK_SUCCESS)
		{
			goto end;
		}
	}

#ifndef NDEBUG
	// Check if validation layers are available.
	const char* const layers[] = {FR_VK_LAYER_KHRONOS_VALIDATION_LAYER_NAME};
	const uint32_t layerCount = FR_LEN(layers);

	bool validationLayersAvailable;
	result = frValidationLayersAvailable(engine->arena, &validationLayersAvailable);
	if(result != VK_SUCCESS)
	{
		goto end;
	}
#endif

	FR_GLOBAL_PFN(vkEnumerateInstanceExtensionProperties);

	uint32_t availableExtensionCount;
	result = vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
	if(result != VK_SUCCESS)
	{
		goto end;
	}
	if(availableExtensionCount < 2)
	{
		result = VK_ERROR_EXTENSION_NOT_PRESENT;
		goto end;
	}
	VkExtensionProperties* availableExtensions = frArenaAllocate(engine->arena, availableExtensionCount * sizeof(availableExtensions[0]), alignof(typeof(availableExtensions[0])));
	if(!availableExtensions)
	{
		result = VK_ERROR_OUT_OF_HOST_MEMORY;
		goto end;
	}
	result = vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions);
	if(result != VK_SUCCESS)
	{
		goto end;
	}

	const char* extensions[4] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
	uint32_t extensionCount = 2;

	if(!frExtensionAvailable(extensions[0], availableExtensions, availableExtensionCount))
	{
		result = VK_ERROR_EXTENSION_NOT_PRESENT;
		goto end;
	}
	if(!frExtensionAvailable(extensions[1], availableExtensions, availableExtensionCount))
	{
		result = VK_ERROR_EXTENSION_NOT_PRESENT;
		goto end;
	}

	if(engine->instanceVersion >= VK_API_VERSION_1_1)
	{
		engine->hasPhysicalDevice2 = true;
	}
	else
	{
		engine->hasPhysicalDevice2 = frExtensionAvailable(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, availableExtensions, availableExtensionCount);
		if(engine->hasPhysicalDevice2)
		{
			extensions[extensionCount] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
			++extensionCount;
		}
	}

#ifndef NDEBUG
	engine->hasDebugUtils = frExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, availableExtensions, availableExtensionCount);

	if(!engine->hasDebugUtils && validationLayersAvailable)
	{
		result = vkEnumerateInstanceExtensionProperties(layers[0], &availableExtensionCount, nullptr);
		if(result != VK_SUCCESS)
		{
			goto end;
		}
		if(availableExtensionCount == 0)
		{
			goto no_extensions;
		}
		availableExtensions = frArenaAllocate(engine->arena, availableExtensionCount * sizeof(availableExtensions[0]), alignof(typeof(availableExtensions[0])));
		if(!availableExtensions)
		{
			result = VK_ERROR_OUT_OF_HOST_MEMORY;
			goto end;
		}
		result = vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions);
		if(result != VK_SUCCESS)
		{
			goto end;
		}

		engine->hasDebugUtils = frExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, availableExtensions, availableExtensionCount);

		no_extensions:
	}

	if(engine->hasDebugUtils)
	{
		extensions[extensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
		++extensionCount;
	}

	const VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {
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

	const VkApplicationInfo applicationInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = name,
		.applicationVersion = version,
		.pEngineName = "Fraus",
		.engineVersion = VK_MAKE_VERSION(FR_VERSION_MAJOR, FR_VERSION_MINOR, FR_VERSION_PATCH),
		.apiVersion = engine->instanceVersion
	};

	const VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &applicationInfo,
#ifndef NDEBUG
		.pNext = engine->hasDebugUtils ? &messengerCreateInfo : nullptr,
		.enabledLayerCount = validationLayersAvailable ? layerCount : 0,
		.ppEnabledLayerNames = layers,
#endif
		.enabledExtensionCount = extensionCount,
		.ppEnabledExtensionNames = extensions
	};

	FR_GLOBAL_PFN(vkCreateInstance);

	result = vkCreateInstance(&createInfo, nullptr, &engine->instance);
	if(result != VK_SUCCESS)
	{
		goto end;
	}

#ifndef NDEBUG
	if(engine->hasDebugUtils)
	{
		FR_INSTANCE_PFN(vkCreateDebugUtilsMessengerEXT);
		result = vkCreateDebugUtilsMessengerEXT(engine->instance, &messengerCreateInfo, nullptr, &engine->messenger);
		if(result != VK_SUCCESS)
		{
			FR_INSTANCE_PFN(vkDestroyInstance);
			vkDestroyInstance(engine->instance, nullptr);
			engine->instance = nullptr;

			goto end;
		}
	}
#endif

	end:
	frArenaRestore(engine->arena, save);
	return result;
}

static VkResult frChoosePhysicalDevice(FrEngine* const engine)
{
	const FrArenaSave save = frArenaSave(engine->arena);

	FR_INSTANCE_PFN(vkEnumeratePhysicalDevices);

	uint32_t physicalDeviceCount;
	VkResult result = vkEnumeratePhysicalDevices(engine->instance, &physicalDeviceCount, nullptr);
	if(result != VK_SUCCESS)
	{
		goto end;
	}
	if(physicalDeviceCount == 0)
	{
		result = VK_ERROR_UNKNOWN;
		goto end;
	}
	VkPhysicalDevice* const physicalDevices = frArenaAllocate(engine->arena, physicalDeviceCount * sizeof(physicalDevices[0]), alignof(typeof(physicalDevices[0])));
	if(!physicalDevices)
	{
		result = VK_ERROR_OUT_OF_HOST_MEMORY;
		goto end;
	}
	result = vkEnumeratePhysicalDevices(engine->instance, &physicalDeviceCount, physicalDevices);
	if(result != VK_SUCCESS)
	{
		goto end;
	}

	PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = nullptr;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = nullptr;
	PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = nullptr;

	PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = nullptr;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties2 vkGetPhysicalDeviceQueueFamilyProperties2 = nullptr;
	PFN_vkGetPhysicalDeviceMemoryProperties2 vkGetPhysicalDeviceMemoryProperties2 = nullptr;

	if(engine->hasPhysicalDevice2)
	{
		vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr(engine->instance, engine->instanceVersion >= VK_API_VERSION_1_1 ? "vkGetPhysicalDeviceProperties2" : "vkGetPhysicalDeviceProperties2KHR");
		vkGetPhysicalDeviceQueueFamilyProperties2 = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2)vkGetInstanceProcAddr(engine->instance, engine->instanceVersion >= VK_API_VERSION_1_1 ? "vkGetPhysicalDeviceQueueFamilyProperties2" : "vkGetPhysicalDeviceQueueFamilyProperties2KHR");
		vkGetPhysicalDeviceMemoryProperties2 = (PFN_vkGetPhysicalDeviceMemoryProperties2)vkGetInstanceProcAddr(engine->instance, engine->instanceVersion >= VK_API_VERSION_1_1 ? "vkGetPhysicalDeviceMemoryProperties2" : "vkGetPhysicalDeviceMemoryProperties2KHR");
	}
	else
	{
		vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(engine->instance, "vkGetPhysicalDeviceProperties");
		vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(engine->instance, "vkGetPhysicalDeviceQueueFamilyProperties");
		vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)vkGetInstanceProcAddr(engine->instance, "vkGetPhysicalDeviceMemoryProperties");
	}

	FR_INSTANCE_PFN(vkGetPhysicalDeviceSurfaceSupportKHR);

	for(uint32_t deviceIndex = 0; deviceIndex < physicalDeviceCount; ++deviceIndex)
	{
		VkPhysicalDeviceProperties2 properties;

		uint32_t familyCount;
		VkQueueFamilyProperties2* families;

		if(engine->hasPhysicalDevice2)
		{
			vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevices[deviceIndex], &familyCount, nullptr);

			families = frArenaAllocate(engine->arena, familyCount * sizeof(families[0]), alignof(typeof(families[0])));
			if(!families)
			{
				result = VK_ERROR_OUT_OF_HOST_MEMORY;
				goto end;
			}

			for(uint32_t familyIndex = 0; familyIndex < familyCount; ++familyIndex)
			{
				families[familyIndex].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
				families[familyIndex].pNext = nullptr;
			}

			vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevices[deviceIndex], &familyCount, families);
		}
		else
		{
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[deviceIndex], &familyCount, nullptr);

			families = frArenaAllocate(engine->arena, familyCount * sizeof(families[0]), alignof(typeof(families[0])));
			if(!families)
			{
				result = VK_ERROR_OUT_OF_HOST_MEMORY;
				goto end;
			}

			VkQueueFamilyProperties* const properties = frArenaAllocate(engine->arena, familyCount * sizeof(properties[0]), alignof(typeof(properties[0])));
			if(!properties)
			{
				result = VK_ERROR_OUT_OF_HOST_MEMORY;
				goto end;
			}

			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[deviceIndex], &familyCount, properties);

			for(uint32_t familyIndex = 0; familyIndex < familyCount; ++familyIndex)
			{
				families[familyIndex].queueFamilyProperties = properties[familyIndex];
			}
		}

		for(uint32_t familyIndex = 0; familyIndex < familyCount; ++familyIndex)
		{
			VkBool32 surfaceSupported;
			result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[deviceIndex], familyIndex, engine->surface, &surfaceSupported);
			if(result != VK_SUCCESS)
			{
				goto end;
			}

			if(families[familyIndex].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT && surfaceSupported)
			{
				if(engine->hasPhysicalDevice2)
				{
					properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
					properties.pNext = nullptr;
					vkGetPhysicalDeviceProperties2(physicalDevices[deviceIndex], &properties);

					engine->memoryProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
					engine->memoryProperties.pNext = nullptr;
					vkGetPhysicalDeviceMemoryProperties2(physicalDevices[deviceIndex], &engine->memoryProperties);
				}
				else
				{
					vkGetPhysicalDeviceProperties(physicalDevices[deviceIndex], &properties.properties);
					vkGetPhysicalDeviceMemoryProperties(physicalDevices[deviceIndex], &engine->memoryProperties.memoryProperties);
				}

				if(deviceIndex > 0 && properties.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					goto next;
				}

				engine->physicalDevice = physicalDevices[deviceIndex];
				engine->queueFamily = familyIndex;

				engine->deviceVersion = properties.properties.apiVersion < engine->instanceVersion ? properties.properties.apiVersion : engine->instanceVersion;

				engine->maxSamplerAnisotropy = properties.properties.limits.maxSamplerAnisotropy;

				const VkSampleCountFlags counts = properties.properties.limits.framebufferColorSampleCounts & properties.properties.limits.framebufferDepthSampleCounts;
				if(counts & VK_SAMPLE_COUNT_64_BIT)
				{
					engine->msaaSamples = VK_SAMPLE_COUNT_64_BIT;
				}
				else if(counts & VK_SAMPLE_COUNT_32_BIT)
				{
					engine->msaaSamples = VK_SAMPLE_COUNT_32_BIT;
				}
				else if(counts & VK_SAMPLE_COUNT_16_BIT)
				{
					engine->msaaSamples = VK_SAMPLE_COUNT_16_BIT;
				}
				else if(counts & VK_SAMPLE_COUNT_8_BIT)
				{
					engine->msaaSamples = VK_SAMPLE_COUNT_8_BIT;
				}
				else if(counts & VK_SAMPLE_COUNT_4_BIT)
				{
					engine->msaaSamples = VK_SAMPLE_COUNT_4_BIT;
				}
				else if(counts & VK_SAMPLE_COUNT_2_BIT)
				{
					engine->msaaSamples = VK_SAMPLE_COUNT_2_BIT;
				}
				else
				{
					engine->msaaSamples = VK_SAMPLE_COUNT_1_BIT;
				}

				if(properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					goto end;
				}
			}
		}

		next:
	}

	end:
	frArenaRestore(engine->arena, save);
	return result;
}

static VkResult frCreateSurface(FrApplication* const application)
{
	const PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(application->engine.instance, "vkCreateWin32SurfaceKHR");

	const VkWin32SurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = windowInstance,
		.hwnd = application->window.window
	};
	return vkCreateWin32SurfaceKHR(application->engine.instance, &createInfo, nullptr, &application->engine.surface);
}

static VkResult frCreateDevice(FrEngine* const engine)
{
	const FrArenaSave save = frArenaSave(engine->arena);

	const float priority = 1.f;

	FR_INSTANCE_PFN(vkEnumerateDeviceExtensionProperties);

	uint32_t availableExtensionCount;
	VkResult result = vkEnumerateDeviceExtensionProperties(engine->physicalDevice, nullptr, &availableExtensionCount, nullptr);
	if(result != VK_SUCCESS)
	{
		goto end;
	}
	if(availableExtensionCount == 0)
	{
		result = VK_ERROR_EXTENSION_NOT_PRESENT;
		goto end;
	}
	VkExtensionProperties* const availableExtensions = frArenaAllocate(engine->arena, availableExtensionCount * sizeof(availableExtensions[0]), alignof(typeof(availableExtensions[0])));
	if(!availableExtensions)
	{
		result = VK_ERROR_OUT_OF_HOST_MEMORY;
		goto end;
	}
	result = vkEnumerateDeviceExtensionProperties(engine->physicalDevice, nullptr, &availableExtensionCount, availableExtensions);
	if(result != VK_SUCCESS)
	{
		goto end;
	}

	const char* extensions[7] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	uint32_t extensionCount = 1;

	if(!frExtensionAvailable(extensions[0], availableExtensions, availableExtensionCount))
	{
		result = VK_ERROR_EXTENSION_NOT_PRESENT;
		goto end;
	}

	if(engine->deviceVersion >= VK_API_VERSION_1_2)
	{
		engine->hasRenderPass2 = true;
	}
	else
	{
		engine->hasRenderPass2 = frExtensionAvailable(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, availableExtensions, availableExtensionCount);
		if(!engine->hasRenderPass2)
		{
			goto no_render_pass_2;
		}

		if(engine->deviceVersion < VK_API_VERSION_1_1)
		{
			if(!engine->hasPhysicalDevice2)
			{
				engine->hasRenderPass2 = false;
				goto no_render_pass_2;
			}

			if(
				!frExtensionAvailable(VK_KHR_MULTIVIEW_EXTENSION_NAME, availableExtensions, availableExtensionCount) ||
				!frExtensionAvailable(VK_KHR_MAINTENANCE2_EXTENSION_NAME, availableExtensions, availableExtensionCount)
			)
			{
				engine->hasRenderPass2 = false;
				goto no_render_pass_2;
			}

			extensions[extensionCount] = VK_KHR_MULTIVIEW_EXTENSION_NAME;
			++extensionCount;
			extensions[extensionCount] = VK_KHR_MAINTENANCE2_EXTENSION_NAME;
			++extensionCount;
		}

		extensions[extensionCount] = VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME;
		++extensionCount;
	}
	no_render_pass_2:

	if(engine->deviceVersion >= VK_API_VERSION_1_3)
	{
		engine->hasDynamicRendering = true;
	}
	else
	{
		engine->hasDynamicRendering = frExtensionAvailable(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, availableExtensions, availableExtensionCount);
		if(!engine->hasDynamicRendering)
		{
			goto no_dynamic_rendering;
		}

		if(engine->deviceVersion < VK_API_VERSION_1_2)
		{
			if(!engine->hasRenderPass2)
			{
				engine->hasDynamicRendering = false;
				goto no_dynamic_rendering;
			}

			if(!frExtensionAvailable(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME, availableExtensions, availableExtensionCount))
			{
				engine->hasDynamicRendering = false;
				goto no_dynamic_rendering;
			}
		}
	}
	no_dynamic_rendering:

	if(engine->deviceVersion >= VK_API_VERSION_1_3)
	{
		engine->hasSynchronization2 = true;
	}
	else
	{
		if(!engine->hasPhysicalDevice2)
		{
			engine->hasSynchronization2 = false;
			goto no_synchronization_2;
		}

		engine->hasSynchronization2 = frExtensionAvailable(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, availableExtensions, availableExtensionCount);
		if(!engine->hasSynchronization2)
		{
			goto no_synchronization_2;
		}
	}
	no_synchronization_2:

	const VkDeviceQueueCreateInfo queueCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = engine->queueFamily,
		.queueCount = 1,
		.pQueuePriorities = &priority
	};

	PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures = nullptr;
	PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2 = nullptr;

	if(engine->hasPhysicalDevice2)
	{
		vkGetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2)vkGetInstanceProcAddr(engine->instance, engine->instanceVersion >= VK_API_VERSION_1_1 ? "vkGetPhysicalDeviceFeatures2" : "vkGetPhysicalDeviceFeatures2KHR");
	}
	else
	{
		vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)vkGetInstanceProcAddr(engine->instance, "vkGetPhysicalDeviceFeatures");
	}

	VkPhysicalDeviceFeatures2 features, wantedFeatures = {};
	VkPhysicalDeviceVulkan13Features features13, wantedFeatures13 = {};
	VkPhysicalDeviceDynamicRenderingFeatures featuresDynamicRendering, wantedFeaturesDynamicRendering = {};
	VkPhysicalDeviceSynchronization2Features featuresSynchronization2, wantedFeaturesSynchronization2 = {};
	if(engine->hasPhysicalDevice2)
	{
		wantedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

		features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		features.pNext = nullptr;

		featuresSynchronization2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		featuresSynchronization2.pNext = nullptr;

		featuresDynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		featuresDynamicRendering.pNext = &featuresSynchronization2;

		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features.pNext = engine->deviceVersion >= VK_API_VERSION_1_3 ? &features13 : (void*)&featuresDynamicRendering;

		vkGetPhysicalDeviceFeatures2(engine->physicalDevice, &features);
	}
	else
	{
		vkGetPhysicalDeviceFeatures(engine->physicalDevice, &features.features);
	}
	wantedFeatures.features.samplerAnisotropy = features.features.samplerAnisotropy;
	wantedFeatures.features.sampleRateShading = features.features.sampleRateShading;
	engine->samplerAnisotropy = features.features.samplerAnisotropy;
	engine->sampleRateShading = features.features.sampleRateShading;

	if(engine->deviceVersion >= VK_API_VERSION_1_3)
	{
		wantedFeatures13.synchronization2 = features13.synchronization2;
		engine->hasSynchronization2 = engine->hasSynchronization2 && features13.synchronization2;

		wantedFeatures13.dynamicRendering = features13.dynamicRendering;
		engine->hasDynamicRendering = engine->hasDynamicRendering && features13.dynamicRendering;

		if(engine->hasDynamicRendering || engine->hasSynchronization2)
		{
			wantedFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
			wantedFeatures13.pNext = nullptr;

			wantedFeatures.pNext = &wantedFeatures13;
		}
	}
	else
	{
		wantedFeaturesSynchronization2.synchronization2 = featuresSynchronization2.synchronization2;
		engine->hasSynchronization2 = engine->hasSynchronization2 && featuresSynchronization2.synchronization2;

		if(engine->hasSynchronization2)
		{
			extensions[extensionCount] = VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME;
			++extensionCount;

			wantedFeaturesSynchronization2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
			wantedFeaturesSynchronization2.pNext = nullptr;
		}

		wantedFeaturesDynamicRendering.dynamicRendering = featuresDynamicRendering.dynamicRendering;
		engine->hasDynamicRendering = engine->hasDynamicRendering && featuresDynamicRendering.dynamicRendering;

		if(engine->hasDynamicRendering)
		{
			if(engine->deviceVersion < VK_API_VERSION_1_2)
			{
				extensions[extensionCount] = VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME;
				++extensionCount;
			}
			extensions[extensionCount] = VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME;
			++extensionCount;

			wantedFeaturesDynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
			wantedFeaturesDynamicRendering.pNext = engine->hasSynchronization2 ? &wantedFeaturesSynchronization2 : nullptr;
		}

		wantedFeatures.pNext = engine->hasDynamicRendering ? &wantedFeaturesDynamicRendering : (void*)(engine->hasSynchronization2 ? &wantedFeaturesSynchronization2 : nullptr);
	}

	const VkDeviceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = engine->hasPhysicalDevice2 ? &wantedFeatures : nullptr,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = extensionCount,
		.ppEnabledExtensionNames = extensions,
		.pEnabledFeatures = engine->hasPhysicalDevice2 ? nullptr : &wantedFeatures.features
	};

	FR_INSTANCE_PFN(vkCreateDevice);
	result = vkCreateDevice(engine->physicalDevice, &createInfo, nullptr, &engine->device);
	if(result != VK_SUCCESS)
	{
		goto end;
	}

	FR_DEVICE_PFN(vkGetDeviceQueue);
	vkGetDeviceQueue(engine->device, engine->queueFamily, 0, &engine->queue);

	end:
	frArenaRestore(engine->arena, save);
	return result;
}

static FrResult frCreateSwapchain(FrEngine* const engine)
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	if(engine->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(engine->physicalDevice, engine->surface, &surfaceCapabilities) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	engine->swapchainExtent = surfaceCapabilities.currentExtent;

	uint32_t surfaceFormatCount;
	if(engine->vkGetPhysicalDeviceSurfaceFormatsKHR(engine->physicalDevice, engine->surface, &surfaceFormatCount, nullptr) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	VkSurfaceFormatKHR* const surfaceFormats = malloc(surfaceFormatCount * sizeof(surfaceFormats));
	if(!surfaceFormats)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	if(engine->vkGetPhysicalDeviceSurfaceFormatsKHR(engine->physicalDevice, engine->surface, &surfaceFormatCount, surfaceFormats) != VK_SUCCESS)
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
	engine->swapchainFormat = surfaceFormats[surfaceFormatIndex].format;

	uint32_t presentModeCount;
	if(engine->vkGetPhysicalDeviceSurfacePresentModesKHR(engine->physicalDevice, engine->surface, &presentModeCount, nullptr) != VK_SUCCESS)
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
	if(engine->vkGetPhysicalDeviceSurfacePresentModesKHR(engine->physicalDevice, engine->surface, &presentModeCount, presentModes) != VK_SUCCESS)
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

	uint32_t minImageCount = FR_FRAMES_IN_FLIGHT + 1;
	if(surfaceCapabilities.maxImageCount < minImageCount)
	{
		minImageCount = surfaceCapabilities.maxImageCount;
	}
	else if(surfaceCapabilities.minImageCount > minImageCount)
	{
		minImageCount = surfaceCapabilities.minImageCount;
	}

	const VkSwapchainCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = engine->surface,
		.minImageCount = minImageCount,
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
		.oldSwapchain = engine->swapchain
	};
	if(engine->vkCreateSwapchainKHR(engine->device, &createInfo, nullptr, &engine->swapchain) != VK_SUCCESS)
	{
		free(surfaceFormats);
		free(presentModes);
		return EXIT_FAILURE;
	}

	free(surfaceFormats);
	free(presentModes);

	if(engine->vkGetSwapchainImagesKHR(engine->device, engine->swapchain, &engine->swapchainImageCount, nullptr) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	VkImage* const newImages = realloc(engine->swapchainImages, engine->swapchainImageCount * sizeof(newImages[0]));
	if(!newImages)
	{
		return FR_ERROR_UNKNOWN;
	}
	engine->swapchainImages = newImages;
	if(engine->vkGetSwapchainImagesKHR(engine->device, engine->swapchain, &engine->swapchainImageCount, engine->swapchainImages) != VK_SUCCESS)
	{
		free(engine->swapchainImages);
		engine->swapchainImages = nullptr;
		return FR_ERROR_UNKNOWN;
	}

	VkImageView* const newImageViews = realloc(engine->swapchainImageViews, engine->swapchainImageCount * sizeof(newImageViews[0]));
	if(!newImageViews)
	{
		free(engine->swapchainImages);
		engine->swapchainImages = nullptr;
		return FR_ERROR_UNKNOWN;
	}
	engine->swapchainImageViews = newImageViews;
	for(uint32_t imageIndex = 0; imageIndex < engine->swapchainImageCount; ++imageIndex)
	{
		if(frCreateImageView(engine, engine->swapchainImages[imageIndex], engine->swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, &engine->swapchainImageViews[imageIndex]) != VK_SUCCESS)
		{
			for(uint32_t j = 0; j < imageIndex; ++j)
			{
				engine->vkDestroyImageView(engine->device, engine->swapchainImageViews[j], nullptr);
			}
			free(engine->swapchainImageViews);
			free(engine->swapchainImages);
			engine->swapchainImageViews = nullptr;
			engine->swapchainImages = nullptr;
			return FR_ERROR_UNKNOWN;
		}
	}

	return FR_SUCCESS;
}

static VkResult frCreateRenderPass(FrEngine* const engine)
{
	if(engine->hasRenderPass2)
	{
		const VkAttachmentDescription2 attachments[] = {
			{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
				.format = engine->swapchainFormat,
				.samples = engine->msaaSamples,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			},
			{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
				.format = VK_FORMAT_D24_UNORM_S8_UINT,
				.samples = engine->msaaSamples,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			},
			{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
				.format = engine->swapchainFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			}
		};

		const VkAttachmentReference2 colorAttachmentReference = {
			.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};
		const VkAttachmentReference2 depthAttachmentReference = {
			.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
		const VkAttachmentReference2 resolveAttachmentReference = {
			.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
			.attachment = 2,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};
		const VkSubpassDescription2 subpass = {
			.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentReference,
			.pResolveAttachments = &resolveAttachmentReference,
			.pDepthStencilAttachment = &depthAttachmentReference
		};

		const VkSubpassDependency2 dependency = {
			.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask =  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
		};

		const VkRenderPassCreateInfo2 createInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
			.attachmentCount = FR_LEN(attachments),
			.pAttachments = attachments,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency
		};

		const PFN_vkCreateRenderPass2 vkCreateRenderPass2 = (PFN_vkCreateRenderPass2)engine->vkGetDeviceProcAddr(engine->device, engine->deviceVersion >= VK_API_VERSION_1_2 ? "vkCreateRenderPass2" : "vkCreateRenderPass2KHR");
		return vkCreateRenderPass2(engine->device, &createInfo, nullptr, &engine->renderPass);
	}
	else
	{
		const VkAttachmentDescription attachments[] = {
			{
				.format = engine->swapchainFormat,
				.samples = engine->msaaSamples,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			},
			{
				.format = VK_FORMAT_D24_UNORM_S8_UINT,
				.samples = engine->msaaSamples,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			},
			{
				.format = engine->swapchainFormat,
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

		FR_DEVICE_PFN(vkCreateRenderPass);
		return vkCreateRenderPass(engine->device, &createInfo, nullptr, &engine->renderPass);
	}
}

static FrResult frCreateFramebuffers(FrEngine* const engine)
{
	VkFramebuffer* const newFramebuffers = realloc(engine->framebuffers, engine->swapchainImageCount * sizeof(newFramebuffers[0]));
	if(!newFramebuffers)
	{
		free(engine->framebuffers);
		return FR_ERROR_UNKNOWN;
	}
	engine->framebuffers = newFramebuffers;

	for(uint32_t imageIndex = 0; imageIndex < engine->swapchainImageCount; ++imageIndex)
	{
		const VkImageView attachments[] = {
			engine->colorImageView,
			engine->depthImageView,
			engine->swapchainImageViews[imageIndex]
		};
		const VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = engine->renderPass,
			.attachmentCount = FR_LEN(attachments),
			.pAttachments = attachments,
			.width = engine->swapchainExtent.width,
			.height = engine->swapchainExtent.height,
			.layers = 1
		};
		if(engine->vkCreateFramebuffer(engine->device, &framebufferInfo, nullptr, &engine->framebuffers[imageIndex]) != VK_SUCCESS)
		{
			for(uint32_t j = 0; j < imageIndex; ++j)
			{
				engine->vkDestroyFramebuffer(engine->device, engine->framebuffers[j], nullptr);
			}

			free(engine->framebuffers);
			engine->framebuffers = nullptr;
			return FR_ERROR_UNKNOWN;
		}
	}

	return FR_SUCCESS;
}

static FrResult frCreateShaderModule(FrEngine* const engine, const char* const path, VkShaderModule* const shaderModule, FrShaderInfo* const info)
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
	
	// Create shader module
	if(engine->vkCreateShaderModule(engine->device, &createInfo, nullptr, shaderModule) != VK_SUCCESS)
	{
		free(code);
		return FR_ERROR_UNKNOWN;
	}

	if(frParseSpirv(code, createInfo.codeSize / sizeof(code[0]), info) != FR_SUCCESS)
	{
		free(code);
		return FR_ERROR_UNKNOWN;
	}

	// Free SPIR-V code
	free(code);

	return FR_SUCCESS;
}

static int frComparePushConstants(const void* const aVoid, const void* const bVoid)
{
	const VkPushConstantRange* const a = aVoid;
	const VkPushConstantRange* const b = bVoid;

	if(a->offset < b->offset)
	{
		return -1;
	}
	if(a->offset > b->offset)
	{
		return 1;
	}
	return 0;
}

FrResult frReserveGraphicsPipelines(FrEngine* const engine, const size_t count)
{
	engine->graphicsPipelines = frArenaAllocate(engine->arena, count * sizeof(engine->graphicsPipelines[0]), alignof(typeof(engine->graphicsPipelines[0])));
	if(!engine->graphicsPipelines)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	return FR_SUCCESS;
}

FrResult frCreateGraphicsPipeline(FrEngine* const engine, const FrPipelineCreateInfo* const createInfo)
{
	// Shader stages
	FrShaderInfo vertexInfo;
	VkShaderModule vertexModule;
	if(frCreateShaderModule(engine, createInfo->vertexShaderPath, &vertexModule, &vertexInfo) != FR_SUCCESS)
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
	if(frCreateShaderModule(engine, createInfo->fragmentShaderPath, &fragmentModule, &fragmentInfo) != FR_SUCCESS)
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
	engine->graphicsPipelines[engine->graphicsPipelineCount].hasPushConstants = pushConstantCount > 0;
	VkPushConstantRange* pushConstants = nullptr;
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

		if(createInfo->vertexInputRateCount && createInfo->vertexInputStrides[bindingI] < offset)
		{
			return FR_ERROR_INVALID_ARGUMENT;
		}
		if(createInfo->vertexInputRateCount && createInfo->vertexInputStrides[bindingI] == offset)
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
	if(createInfo->vertexInputRateCount)
	{
		inputBindingCount = createInfo->vertexInputRateCount;
		inputBindings = malloc(inputBindingCount * sizeof(inputBindings[0]));
		if(!inputBindings)
		{
			free(bindings);
			free(attributes);
			free(pushConstants);
			return FR_ERROR_OUT_OF_HOST_MEMORY;
		}

		uint32_t s = 0;
		for(uint32_t i = 0; i < inputBindingCount; s += createInfo->vertexInputStrides[i], ++i)
		{
			inputBindings[i].binding = i;
			inputBindings[i].stride = createInfo->vertexInputStrides[i];
			inputBindings[i].inputRate = createInfo->vertexInputRates[i];
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
	const VkPipelineMultisampleStateCreateInfo multisample = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = engine->msaaSamples,
		.sampleShadingEnable = engine->sampleRateShading,
		.minSampleShading = 1.f
	};

	// Depth stencil
	const VkPipelineDepthStencilStateCreateInfo depthStencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = createInfo->depthTestDisable ? VK_FALSE : VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE
	};

	// Color blend
	const VkPipelineColorBlendAttachmentState colorAttachment = {
		.blendEnable = createInfo->alphaBlendEnable ? VK_TRUE : VK_FALSE,
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

	engine->graphicsPipelines[engine->graphicsPipelineCount].descriptorTypeCount = bindingCount;
	engine->graphicsPipelines[engine->graphicsPipelineCount].descriptorTypes = malloc(bindingCount * sizeof(engine->graphicsPipelines[engine->graphicsPipelineCount].descriptorTypes[0]));
	if(!engine->graphicsPipelines[engine->graphicsPipelineCount].descriptorTypes)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	for(uint32_t i = 0; i < bindingCount; ++i)
	{
		engine->graphicsPipelines[engine->graphicsPipelineCount].descriptorTypes[i] = bindings[i].descriptorType;
	}

	const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = bindingCount,
		.pBindings = bindings
	};
	if(engine->vkCreateDescriptorSetLayout(
		engine->device,
		&descriptorSetLayoutInfo,
		nullptr,
		&engine->graphicsPipelines[engine->graphicsPipelineCount].descriptorSetLayout
	) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	free(bindings);

	// Layout
	const VkPipelineLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &engine->graphicsPipelines[engine->graphicsPipelineCount].descriptorSetLayout,
		.pushConstantRangeCount = pushConstantCount,
		.pPushConstantRanges = pushConstants
	};

	if(engine->vkCreatePipelineLayout(
		engine->device,
		&layoutInfo,
		nullptr,
		&engine->graphicsPipelines[engine->graphicsPipelineCount].pipelineLayout
	) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	free(pushConstants);

	const VkPipelineRenderingCreateInfo renderingInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &engine->swapchainFormat,
		.depthAttachmentFormat = VK_FORMAT_D24_UNORM_S8_UINT
	};

	// Pipeline
	const VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = engine->hasDynamicRendering ? &renderingInfo : nullptr,
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
		.layout = engine->graphicsPipelines[engine->graphicsPipelineCount].pipelineLayout,
		.renderPass = engine->renderPass,
		.subpass = 0
	};

	if(engine->vkCreateGraphicsPipelines(
		engine->device,
		nullptr,
		1,
		&pipelineInfo,
		nullptr,
		&engine->graphicsPipelines[engine->graphicsPipelineCount].pipeline
	) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	free(attributes);

	engine->vkDestroyShaderModule(engine->device, vertexModule, nullptr);
	engine->vkDestroyShaderModule(engine->device, fragmentModule, nullptr);

	++engine->graphicsPipelineCount;

	return FR_SUCCESS;
}

FrResult frReserveUniformBuffers(FrEngine* const engine, const size_t count)
{
	engine->uniformBuffers = frArenaAllocate(engine->arena, count * sizeof(engine->uniformBuffers[0]), alignof(typeof(engine->uniformBuffers[0])));
	if(!engine->uniformBuffers)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	return frCreateUniformBuffer(engine, sizeof(float[16]));
}

FrResult frCreateUniformBuffer(FrEngine* const engine, const VkDeviceSize size)
{
	engine->uniformBuffers[engine->uniformBufferCount].buffersSize = size;

	// Create uniform buffers
	for(uint32_t frameIndex = 0; frameIndex < FR_FRAMES_IN_FLIGHT; ++frameIndex)
	{
		if(frCreateBuffer(
			engine,
			size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&engine->uniformBuffers[engine->uniformBufferCount].buffers[frameIndex],
			&engine->uniformBuffers[engine->uniformBufferCount].bufferMemories[frameIndex]
		) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}

		if(engine->vkMapMemory(engine->device, engine->uniformBuffers[engine->uniformBufferCount].bufferMemories[frameIndex], 0, size, 0, &engine->uniformBuffers[engine->uniformBufferCount].bufferDatas[frameIndex]) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}

	++engine->uniformBufferCount;

	return FR_SUCCESS;
}

FrResult frReserveStorageBuffers(FrEngine* const engine, const size_t count)
{
	engine->storageBuffers = frArenaAllocate(engine->arena, count * sizeof(engine->storageBuffers[0]), alignof(typeof(engine->storageBuffers[0])));
	if(!engine->storageBuffers)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	return FR_SUCCESS;
}

FrResult frCreateStorageBuffer(FrEngine* const engine, const VkDeviceSize size)
{
	engine->storageBuffers[engine->storageBufferCount].bufferSize = size;

	// Create storage buffer
	if(frCreateBuffer(
		engine,
		size,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&engine->storageBuffers[engine->storageBufferCount].buffer,
		&engine->storageBuffers[engine->storageBufferCount].bufferMemory
	) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	++engine->storageBufferCount;

	return FR_SUCCESS;
}

FrResult frSetStorageBufferData(FrEngine* const engine, const uint32_t storageBufferIndex, const void* const data, const VkDeviceSize size)
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	if(frCreateBuffer(
		engine,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		&stagingBufferMemory
	) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	void* mappedData;
	if(engine->vkMapMemory(engine->device, stagingBufferMemory, 0, size, 0, &mappedData) != VK_SUCCESS)
	{
		engine->vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
		engine->vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
		return FR_ERROR_UNKNOWN;
	}
	memcpy(mappedData, data, size);
	engine->vkUnmapMemory(engine->device, stagingBufferMemory);

	if(frCopyBuffer(engine, stagingBuffer, engine->storageBuffers[storageBufferIndex].buffer, size) != VK_SUCCESS)
	{
		engine->vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
		engine->vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
		return FR_ERROR_UNKNOWN;
	}

	engine->vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
	engine->vkFreeMemory(engine->device, stagingBufferMemory, nullptr);

	return FR_SUCCESS;
}

FrResult frReserveTextures(FrEngine* const engine, const size_t count)
{
	engine->textures = frArenaAllocate(engine->arena, count * sizeof(engine->textures[0]), alignof(typeof(engine->textures[0])));
	if(!engine->textures)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	return FR_SUCCESS;
}

FrResult frReserveObjects(FrEngine* const engine, const size_t count)
{
	engine->objects = frArenaAllocate(engine->arena, count * sizeof(engine->objects[0]), alignof(typeof(engine->objects[0])));
	if(!engine->objects)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	return FR_SUCCESS;
}

static VkResult frCreateSampler(FrEngine* const engine)
{
	const VkSamplerCreateInfo samplerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.f,
		.anisotropyEnable = engine->samplerAnisotropy,
		.maxAnisotropy = engine->maxSamplerAnisotropy,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.f,
		.maxLod = VK_LOD_CLAMP_NONE,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};

	FR_DEVICE_PFN(vkCreateSampler);
	return vkCreateSampler(engine->device, &samplerCreateInfo, nullptr, &engine->textureSampler);
}

static VkResult frCreateAttachments(FrEngine* const engine)
{
	engine->vkDestroyImageView(engine->device, engine->depthImageView, nullptr);
	engine->vkDestroyImage(engine->device, engine->depthImage, nullptr);
	engine->vkDestroyImageView(engine->device, engine->colorImageView, nullptr);
	engine->vkDestroyImage(engine->device, engine->colorImage, nullptr);

	const VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;

	VkFormatProperties2 properties;
	if(engine->hasPhysicalDevice2)
	{
		properties.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
		properties.pNext = nullptr;
		engine->vkGetPhysicalDeviceFormatProperties2(engine->physicalDevice, depthFormat, &properties);
	}
	else
	{
		engine->vkGetPhysicalDeviceFormatProperties(engine->physicalDevice, depthFormat, &properties.formatProperties);
	}
	if(!(properties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
	{
		return VK_ERROR_FORMAT_NOT_SUPPORTED;
	}

	VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = engine->swapchainFormat,
		.extent = {
			.width = engine->swapchainExtent.width,
			.height = engine->swapchainExtent.height,
			.depth = 1
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = engine->msaaSamples,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	VkResult result = engine->vkCreateImage(engine->device, &imageInfo, nullptr, &engine->colorImage);
	if(result != VK_SUCCESS)
	{
		return result;
	}

	imageInfo.format = depthFormat;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	result = engine->vkCreateImage(engine->device, &imageInfo, nullptr, &engine->depthImage);
	if(result != VK_SUCCESS)
	{
		return result;
	}

	VkMemoryRequirements memoryRequirements[2];
	engine->vkGetImageMemoryRequirements(engine->device, engine->colorImage, &memoryRequirements[0]);
	engine->vkGetImageMemoryRequirements(engine->device, engine->depthImage, &memoryRequirements[1]);

	const unsigned char largestAlignement = memoryRequirements[0].alignment > memoryRequirements[1].alignment ? 0 : 1;
	const VkDeviceSize size = memoryRequirements[largestAlignement].size + memoryRequirements[1 - largestAlignement].size;

	if(size > engine->attachmentsMemorySize)
	{
		engine->vkFreeMemory(engine->device, engine->attachmentsMemory, nullptr);

		engine->attachmentsMemorySize = size;

		const uint32_t memoryTypeBits = memoryRequirements[0].memoryTypeBits & memoryRequirements[1].memoryTypeBits;

		VkMemoryAllocateInfo memoryInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = engine->attachmentsMemorySize
		};
		result = frFindMemoryTypeIndex(engine, memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryInfo.memoryTypeIndex);
		if(result != VK_SUCCESS)
		{
			return result;
		}
	
		result = engine->vkAllocateMemory(engine->device, &memoryInfo, nullptr, &engine->attachmentsMemory);
		if(result != VK_SUCCESS)
		{
			return result;
		}
	}

	result = engine->vkBindImageMemory(engine->device, engine->colorImage, engine->attachmentsMemory, largestAlignement == 0 ? 0 : memoryRequirements[1].size);
	if(result != VK_SUCCESS)
	{
		return result;
	}
	result = engine->vkBindImageMemory(engine->device, engine->depthImage, engine->attachmentsMemory, largestAlignement == 1 ? 0 : memoryRequirements[0].size);
	if(result != VK_SUCCESS)
	{
		return result;
	}

	result = frCreateImageView(engine, engine->colorImage, engine->swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, &engine->colorImageView);
	if(result != VK_SUCCESS)
	{
		return result;
	}

	result = frCreateImageView(engine, engine->depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, &engine->depthImageView);
	if(result != VK_SUCCESS)
	{
		return result;
	}

	return result;
}

static VkResult frCreateCommandPools(FrEngine* const engine)
{
	engine->renderFinishedSemaphores = malloc(engine->swapchainImageCount * sizeof(engine->renderFinishedSemaphores[0]));
	if(!engine->renderFinishedSemaphores)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	const VkSemaphoreCreateInfo semaphoreCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};
	const VkFenceCreateInfo fenceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	VkResult result;

	for(uint32_t imageIndex = 0; imageIndex < engine->swapchainImageCount; ++imageIndex)
	{
		result = engine->vkCreateSemaphore(engine->device, &semaphoreCreateInfo, nullptr, &engine->renderFinishedSemaphores[imageIndex]);
		if(result != VK_SUCCESS)
		{
			return result;
		}
	}

	engine->frameInFlightIndex = 0;

	FR_DEVICE_PFN(vkCreateCommandPool);
	FR_DEVICE_PFN(vkCreateFence);

	for(uint32_t i = 0; i < FR_FRAMES_IN_FLIGHT; ++i)
	{
		const VkCommandPoolCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = engine->queueFamily
		};
		result = vkCreateCommandPool(engine->device, &createInfo, nullptr, &engine->commandPools[i]);
		if(result != VK_SUCCESS)
		{
			return result;
		}

		const VkCommandBufferAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = engine->commandPools[i],
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};
		result = engine->vkAllocateCommandBuffers(engine->device, &allocateInfo, &engine->commandBuffers[i]);
		if(result != VK_SUCCESS)
		{
			return result;
		}

		result = engine->vkCreateSemaphore(engine->device, &semaphoreCreateInfo, nullptr, &engine->imageAvailableSemaphores[i]);
		if(result != VK_SUCCESS)
		{
			return result;
		}
		result = vkCreateFence(engine->device, &fenceCreateInfo, nullptr, &engine->frameInFlightFences[i]);
		if(result != VK_SUCCESS)
		{
			return result;
		}
	}

	return result;
}

FrResult frDrawFrame(FrApplication* const application)
{
	// Wait for fence
	if(application->engine.vkWaitForFences(application->engine.device, 1, &application->engine.frameInFlightFences[application->engine.frameInFlightIndex], VK_TRUE, UINT64_MAX) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(application->engine.vkResetFences(application->engine.device, 1, &application->engine.frameInFlightFences[application->engine.frameInFlightIndex]) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Acquire image
	uint32_t swapchainImageIndex;
	VkResult result = application->engine.vkAcquireNextImageKHR(application->engine.device, application->engine.swapchain, UINT64_MAX, application->engine.imageAvailableSemaphores[application->engine.frameInFlightIndex], nullptr, &swapchainImageIndex);
	if(result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		application->window.resized = true;
		return FR_SUCCESS;
	}
	else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Reset command pool
	if(application->engine.vkResetCommandPool(application->engine.device, application->engine.commandPools[application->engine.frameInFlightIndex], 0) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Begin command buffer and render pass
	const VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	if(application->engine.vkBeginCommandBuffer(application->engine.commandBuffers[application->engine.frameInFlightIndex], &beginInfo) != VK_SUCCESS)
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
	if(application->engine.hasDynamicRendering)
	{
		if(application->engine.hasSynchronization2)
		{
			const VkImageMemoryBarrier2 barriers[] = {
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = application->engine.colorImage,
					.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.subresourceRange.levelCount = 1,
					.subresourceRange.layerCount = 1
				},
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = application->engine.swapchainImages[swapchainImageIndex],
					.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.subresourceRange.levelCount = 1,
					.subresourceRange.layerCount = 1
				},
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
					.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
					.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = application->engine.depthImage,
					.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
					.subresourceRange.levelCount = 1,
					.subresourceRange.layerCount = 1
				}
			};

			const VkDependencyInfo dependency = {
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.imageMemoryBarrierCount = FR_LEN(barriers),
				.pImageMemoryBarriers = barriers
			};

			application->engine.vkCmdPipelineBarrier2(application->engine.commandBuffers[application->engine.frameInFlightIndex], &dependency);
		}
		else
		{
			const VkImageMemoryBarrier barriers[] = {
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = application->engine.colorImage,
					.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.subresourceRange.levelCount = 1,
					.subresourceRange.layerCount = 1
				},
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = application->engine.swapchainImages[swapchainImageIndex],
					.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.subresourceRange.levelCount = 1,
					.subresourceRange.layerCount = 1
				},
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = application->engine.depthImage,
					.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
					.subresourceRange.levelCount = 1,
					.subresourceRange.layerCount = 1
				}
			};

			application->engine.vkCmdPipelineBarrier(
				application->engine.commandBuffers[application->engine.frameInFlightIndex],
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				2,
				&barriers[0]
			);
			application->engine.vkCmdPipelineBarrier(
				application->engine.commandBuffers[application->engine.frameInFlightIndex],
				VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barriers[2]
			);
		}

		const VkRenderingAttachmentInfo attachmentInfos[] = {
			{
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.imageView = application->engine.colorImageView,
				.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
				.resolveImageView = application->engine.swapchainImageViews[swapchainImageIndex],
				.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.clearValue = clearColor
			},
			{
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.imageView = application->engine.depthImageView,
				.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.clearValue = clearDepthStencil
			}
		};
		const VkRenderingInfo renderingInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea.offset = {0, 0},
			.renderArea.extent = application->engine.swapchainExtent,
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachmentInfos[0],
			.pDepthAttachment = &attachmentInfos[1]
		};

		application->engine.vkCmdBeginRendering(application->engine.commandBuffers[application->engine.frameInFlightIndex], &renderingInfo);
	}
	else
	{
		const VkRenderPassBeginInfo renderPassBegin = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = application->engine.renderPass,
			.framebuffer = application->engine.framebuffers[swapchainImageIndex],
			.renderArea.offset = {0, 0},
			.renderArea.extent = application->engine.swapchainExtent,
			.clearValueCount = FR_LEN(clearValues),
			.pClearValues = clearValues
		};

		if(application->engine.hasRenderPass2)
		{
			const VkSubpassBeginInfo subpassBegin = {
				.sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
				.contents = VK_SUBPASS_CONTENTS_INLINE
			};
			application->engine.vkCmdBeginRenderPass2(application->engine.commandBuffers[application->engine.frameInFlightIndex], &renderPassBegin, &subpassBegin);
		}
		else
		{
			application->engine.vkCmdBeginRenderPass(application->engine.commandBuffers[application->engine.frameInFlightIndex], &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
		}
	}

	// Update camera
	float cameraMatrix[16];
	frGetCameraMatrix(&application->camera, &application->engine, cameraMatrix);
	memcpy(application->engine.uniformBuffers[0].bufferDatas[application->engine.frameInFlightIndex], cameraMatrix, sizeof(cameraMatrix));

	const VkViewport viewport = {
		.x = 0.f,
		.y = 0.f,
		.width = (float)application->engine.swapchainExtent.width,
		.height = (float)application->engine.swapchainExtent.height,
		.minDepth = 0.f,
		.maxDepth = 1.f
	};
	application->engine.vkCmdSetViewport(application->engine.commandBuffers[application->engine.frameInFlightIndex], 0, 1, &viewport);

	const VkRect2D scissor = {
		.offset = {0, 0},
		.extent = application->engine.swapchainExtent
	};
	application->engine.vkCmdSetScissor(application->engine.commandBuffers[application->engine.frameInFlightIndex], 0, 1, &scissor);

	const VkDeviceSize offsets[] = {0, 0};
	static uint32_t lastPipelineIndex = UINT32_MAX;

	for(uint32_t objectIndex = 0; objectIndex < application->engine.objectCount; ++objectIndex)
	{
		const FrVulkanObject* const object = &application->engine.objects[objectIndex];

		if(object->pipelineIndex != lastPipelineIndex)
		{
			application->engine.vkCmdBindPipeline(application->engine.commandBuffers[application->engine.frameInFlightIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, application->engine.graphicsPipelines[object->pipelineIndex].pipeline);
			lastPipelineIndex = object->pipelineIndex;
		}

		application->engine.vkCmdBindDescriptorSets(application->engine.commandBuffers[application->engine.frameInFlightIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, application->engine.graphicsPipelines[object->pipelineIndex].pipelineLayout, 0, 1, &object->descriptorSets[application->engine.frameInFlightIndex], 0, nullptr);
		if(application->engine.graphicsPipelines[object->pipelineIndex].hasPushConstants)
		{
			application->engine.vkCmdPushConstants(application->engine.commandBuffers[application->engine.frameInFlightIndex], application->engine.graphicsPipelines[object->pipelineIndex].pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(object->transformation), object->transformation);
		}

		const VkBuffer buffers[] = {object->buffer, object->instanceBuffer};
		application->engine.vkCmdBindVertexBuffers(application->engine.commandBuffers[application->engine.frameInFlightIndex], 0, 1 + (object->instanceCount > 1), buffers, offsets);
		application->engine.vkCmdBindIndexBuffer(application->engine.commandBuffers[application->engine.frameInFlightIndex], object->buffer, object->vertexCount * sizeof(object->vertices[0]), VK_INDEX_TYPE_UINT32);

		application->engine.vkCmdDrawIndexed(application->engine.commandBuffers[application->engine.frameInFlightIndex], object->indexCount, object->instanceCount, 0, 0, 0);
	}

	// End render pass and command buffer
	if(application->engine.hasDynamicRendering)
	{
		application->engine.vkCmdEndRendering(application->engine.commandBuffers[application->engine.frameInFlightIndex]);

		if(application->engine.hasSynchronization2)
		{
			const VkImageMemoryBarrier2 barrier = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstAccessMask = 0,
				.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = application->engine.swapchainImages[swapchainImageIndex],
				.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.subresourceRange.levelCount = 1,
				.subresourceRange.layerCount = 1
			};

			const VkDependencyInfo dependency = {
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &barrier
			};

			application->engine.vkCmdPipelineBarrier2(application->engine.commandBuffers[application->engine.frameInFlightIndex], &dependency);
		}
		else
		{
			const VkImageMemoryBarrier barrier = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = 0,
				.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = application->engine.swapchainImages[swapchainImageIndex],
				.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.subresourceRange.levelCount = 1,
				.subresourceRange.layerCount = 1
			};

			application->engine.vkCmdPipelineBarrier(
				application->engine.commandBuffers[application->engine.frameInFlightIndex],
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barrier
			);
		}
	}
	else if(application->engine.hasRenderPass2)
	{
		const VkSubpassEndInfo subpassEnd = {
			.sType = VK_STRUCTURE_TYPE_SUBPASS_END_INFO
		};
		application->engine.vkCmdEndRenderPass2(application->engine.commandBuffers[application->engine.frameInFlightIndex], &subpassEnd);
	}
	else
	{
		application->engine.vkCmdEndRenderPass(application->engine.commandBuffers[application->engine.frameInFlightIndex]);
	}

	if(application->engine.vkEndCommandBuffer(application->engine.commandBuffers[application->engine.frameInFlightIndex]) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	if(application->engine.hasSynchronization2)
	{
		const VkSemaphoreSubmitInfo waitSemaphoreInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = application->engine.imageAvailableSemaphores[application->engine.frameInFlightIndex],
			.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
		};
		const VkCommandBufferSubmitInfo commandBufferInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = application->engine.commandBuffers[application->engine.frameInFlightIndex]
		};
		const VkSemaphoreSubmitInfo signalSemahporeInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = application->engine.renderFinishedSemaphores[swapchainImageIndex],
			.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
		};
		const VkSubmitInfo2 submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = &waitSemaphoreInfo,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &commandBufferInfo,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &signalSemahporeInfo
		};
		if(application->engine.vkQueueSubmit2(application->engine.queue, 1, &submitInfo, application->engine.frameInFlightFences[application->engine.frameInFlightIndex]) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}
	else
	{
		const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		const VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &application->engine.imageAvailableSemaphores[application->engine.frameInFlightIndex],
			.pWaitDstStageMask = &waitStage,
			.commandBufferCount = 1,
			.pCommandBuffers = &application->engine.commandBuffers[application->engine.frameInFlightIndex],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &application->engine.renderFinishedSemaphores[swapchainImageIndex]
		};
		if(application->engine.vkQueueSubmit(application->engine.queue, 1, &submitInfo, application->engine.frameInFlightFences[application->engine.frameInFlightIndex]) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}

	VkResult presentResult;
	const VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &application->engine.renderFinishedSemaphores[swapchainImageIndex],
		.swapchainCount = 1,
		.pSwapchains = &application->engine.swapchain,
		.pImageIndices = &swapchainImageIndex,
		.pResults = &presentResult
	};
	result = application->engine.vkQueuePresentKHR(application->engine.queue, &presentInfo);
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		application->window.resized = true;
	}
	else if(result != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	application->engine.frameInFlightIndex = (application->engine.frameInFlightIndex + 1) % FR_FRAMES_IN_FLIGHT;

	return FR_SUCCESS;
}

FrResult frRecreateSwapchain(FrApplication* const application)
{
	if(application->engine.vkDeviceWaitIdle(application->engine.device) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	const VkSwapchainKHR oldSwapchain = application->engine.swapchain;

	for(uint32_t imageIndex = 0; imageIndex < application->engine.swapchainImageCount; ++imageIndex)
	{
		if(!application->engine.hasDynamicRendering)
		{
			application->engine.vkDestroyFramebuffer(application->engine.device, application->engine.framebuffers[imageIndex], nullptr);
		}
		application->engine.vkDestroyImageView(application->engine.device, application->engine.swapchainImageViews[imageIndex], nullptr);
	}

	for(uint32_t i = 0; i < application->engine.swapchainImageCount; ++i)
	{
		application->engine.vkDestroySemaphore(application->engine.device, application->engine.renderFinishedSemaphores[i], nullptr);
	}

	if(frCreateSwapchain(&application->engine) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(frCreateAttachments(&application->engine) != VK_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}
	if(!application->engine.hasDynamicRendering)
	{
		if(frCreateFramebuffers(&application->engine) != FR_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}

	VkSemaphore* const newRenderFinishedSemaphores = realloc(application->engine.renderFinishedSemaphores, application->engine.swapchainImageCount * sizeof(newRenderFinishedSemaphores[0]));
	if(!newRenderFinishedSemaphores)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	application->engine.renderFinishedSemaphores = newRenderFinishedSemaphores;

	const VkSemaphoreCreateInfo semaphoreInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};
	for(uint32_t i = 0; i < application->engine.swapchainImageCount; ++i)
	{
		if(application->engine.vkCreateSemaphore(application->engine.device, &semaphoreInfo, nullptr, &application->engine.renderFinishedSemaphores[i]) != VK_SUCCESS)
		{
			return FR_ERROR_UNKNOWN;
		}
	}

	application->engine.vkDestroySwapchainKHR(application->engine.device, oldSwapchain, nullptr);

	application->window.resized = false;

	return FR_SUCCESS;
}
