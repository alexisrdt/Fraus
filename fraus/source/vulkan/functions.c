#include "./functions.h"

#define FR_DECLARE_PFN(name) \
PFN_##name name;

FR_PFNS(FR_DECLARE_PFN)

#define FR_LOAD_GLOBAL_PFN(name) \
name = (PFN_##name)vkGetInstanceProcAddr(NULL, #name); \
if(!name) \
{ \
	return FR_ERROR_UNKNOWN; \
}

FrResult frLoadGlobalFunctions(void)
{
	FR_GENERAL_PFNS(FR_LOAD_GLOBAL_PFN)

	return FR_SUCCESS;
}

#define FR_LOAD_INSTANCE_PFN(name) \
name = (PFN_##name)vkGetInstanceProcAddr(instance, #name); \
if(!name) \
{ \
	return FR_ERROR_UNKNOWN; \
}

FrResult frLoadInstanceFunctions(void)
{
	#ifndef NDEBUG
	if(debugExtensionAvailable)
	{
		FR_DEBUG_INSTANCE_PFNS(FR_LOAD_INSTANCE_PFN)
	}
	#endif

	FR_BASE_INSTANCE_PFNS(FR_LOAD_INSTANCE_PFN)

	return FR_SUCCESS;
}

#define FR_LOAD_DEVICE_PFN(name) \
name = (PFN_##name)vkGetDeviceProcAddr(device, #name); \
if(!name) \
{ \
	return FR_ERROR_UNKNOWN; \
}

FrResult frLoadDeviceFunctions(void)
{
	FR_DEVICE_PFNS(FR_LOAD_DEVICE_PFN)

	return FR_SUCCESS;
}
