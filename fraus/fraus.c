#include "fraus.h"

#include "vulkan/functions.h"

#ifdef _WIN32
#include <windows.h>

static HMODULE vulkanLibrary;
#endif

FrResult frInit()
{

#ifdef _WIN32

	vulkanLibrary = LoadLibrary(TEXT("vulkan-1"));
	if(!vulkanLibrary) return FR_ERROR_FILE_NOT_FOUND;

	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vulkanLibrary, "vkGetInstanceProcAddr");
	if(!vkGetInstanceProcAddr) return FR_ERROR_UNKNOWN;

#endif

	FR_LOAD_GLOBAL_PFN(vkEnumerateInstanceLayerProperties)
	FR_LOAD_GLOBAL_PFN(vkEnumerateInstanceExtensionProperties)
	FR_LOAD_GLOBAL_PFN(vkCreateInstance)

	return FR_SUCCESS;
}

FrResult frFinish()
{

#ifdef _WIN32

	if(!FreeLibrary(vulkanLibrary)) return FR_ERROR_UNKNOWN;

#endif

	return FR_SUCCESS;
}
