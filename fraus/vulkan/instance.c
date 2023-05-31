#include "instance.h"

#include "functions.h"

FrResult frCreateInstance(const char* pName, uint32_t version, VkInstance* pInstance)
{
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = pName,
		.applicationVersion = version,
		.pEngineName = "Fraus",
		.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 1),
		.apiVersion = VK_API_VERSION_1_0 // TODO: dynamically choose Vulkan version
	};

	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &application_info
	};

	if(vkCreateInstance(&create_info, NULL, pInstance) != VK_SUCCESS) return FR_ERROR_UNKNOWN;

	FR_LOAD_INSTANCE_PFN(*pInstance, vkDestroyInstance)
	FR_LOAD_INSTANCE_PFN(*pInstance, vkEnumeratePhysicalDevices)

	return FR_SUCCESS;
}
