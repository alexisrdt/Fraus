#include "fraus.h"

#include "vulkan/functions.h"

#ifdef _WIN32
#include <windows.h>

static HMODULE vulkan_library;
#endif

FrResult frInit()
{
	#ifdef _WIN32
	vulkan_library = LoadLibrary(TEXT("vulkan-1"));
	if(!vulkan_library) return FR_ERROR_FILE_NOT_FOUND; // TODO: check doc

	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vulkan_library, "vkGetInstanceProcAddr");
	if(!vkGetInstanceProcAddr) return FR_ERROR_UNKNOWN;
	#endif

	FR_LOAD_GLOBAL_PFN(vkCreateInstance)

	return FR_SUCCESS;
}

FrResult frFinish()
{
	#ifdef _WIN32
	if(!FreeLibrary(vulkan_library)) return FR_ERROR_UNKNOWN; // TODO: check doc
	#endif

	return FR_SUCCESS;
}
