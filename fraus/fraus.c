#include "fraus.h"

#include <stdlib.h>

#ifdef _WIN32
	#include <windows.h>

	#ifndef NDEBUG
		#include <crtdbg.h>
	#endif

	static HMODULE frVulkanLibrary = NULL;
#else
	#include <dlfcn.h>

	static void* frVulkanLibrary = NULL;
#endif

static uint32_t frApplicationCount = 0;

/*
 * Initialize Fraus
 * Returns:
 * - FR_SUCCESS if everything went well
 * - FR_ERROR_FILE_NOT_FOUND if the Vulkan library could not be found
 * - FR_ERROR_UNKNOWN if some other error occured
 */
FrResult frCreateApplication(const char* pName, uint32_t version, FrApplication* pApplication)
{
	if(!frVulkanLibrary)
	{
#ifdef _WIN32
		frVulkanLibrary = LoadLibrary(TEXT("vulkan-1"));
		if(!frVulkanLibrary) return FR_ERROR_FILE_NOT_FOUND;

		vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(frVulkanLibrary, "vkGetInstanceProcAddr");
		if(!vkGetInstanceProcAddr) return FR_ERROR_UNKNOWN;
#else
	#if defined(__APPLE__) || defined(__MACH__)
		dlopen("libvulkan.dylib", RTLD_LAZY);
	#else
		dlopen("libvulkan.so", RTLD_LAZY);
	#endif
		if(!frVulkanLibrary) return FR_ERROR_FILE_NOT_FOUND;

		vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(frVulkanLibrary, "vkGetInstanceProcAddr");
		if(!vkGetInstanceProcAddr) return FR_ERROR_UNKNOWN;
#endif
		FR_LOAD_GLOBAL_PFN(vkEnumerateInstanceLayerProperties)
		FR_LOAD_GLOBAL_PFN(vkEnumerateInstanceExtensionProperties)
		FR_LOAD_GLOBAL_PFN(vkCreateInstance)
	}

	if(frCreateWindow(pName, &pApplication->window) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	pApplication->vulkanData.pApplication = pApplication;
	if(frCreateVulkanData(&pApplication->vulkanData, pName, version) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	frCreateCamera(&pApplication->camera, &pApplication->vulkanData);

	++frApplicationCount;

	return FR_SUCCESS;
}

/*
 * Clean up Fraus
 * Returns:
 * - FR_SUCCESS if everything went well
 * - FR_ERROR_UNKNOWN if freeing the Vulkan library failed
 */
FrResult frDestroyApplication(FrApplication* pApplication)
{
	frDestroyWindow(&pApplication->window);
	if(frDestroyVulkanData(&pApplication->vulkanData) != FR_SUCCESS) return FR_ERROR_UNKNOWN;

	if(--frApplicationCount > 0) return FR_SUCCESS;

#ifdef _WIN32
	if(!FreeLibrary(frVulkanLibrary)) return FR_ERROR_UNKNOWN;

	#ifndef NDEBUG
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);

		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);

		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

		_CrtDumpMemoryLeaks();
	#endif
#else
	if(dlclose(frVulkanLibrary) != 0) return FR_ERROR_UNKNOWN;
#endif

	return FR_SUCCESS;
}

/*
 * Main loop of the application
 * - pApplication: pointer to an application
 * Returns the exit value of the application
 */
int frMainLoop(FrApplication* pApplication)
{
#ifdef _WIN32
	int returnValue;
	RECT clientRect;
	MSG message;
	while(true)
	{
		// Handle events
		while(PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
		{
			// If the message is a quit message, return
			// (also return the quit value if the pointer to handle isn't NULL)
			if(message.message == WM_QUIT)
			{
				returnValue = (int)message.wParam;
				goto end;
			}

			// Translate and dispatch message to window
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		// Render
		if(pApplication->window.resized)
		{
			GetClientRect(pApplication->window.handle, &clientRect);
			if(clientRect.right > 0 && clientRect.bottom > 0)
			{
				if(frRecreateSwapchain(&pApplication->vulkanData) != FR_SUCCESS)
				{
					returnValue = EXIT_FAILURE;
					goto end;
				}

				if(frDrawFrame(&pApplication->vulkanData) != FR_SUCCESS)
				{
					returnValue = EXIT_FAILURE;
					goto end;
				}
			}
		}
		else
		{
			if(frDrawFrame(&pApplication->vulkanData) != FR_SUCCESS)
			{
				returnValue = EXIT_FAILURE;
				goto end;
			}
		}
	}
	end:

	pApplication->vulkanData.threadShouldExit = true;

	if(pApplication->vulkanData.functions.vkDeviceWaitIdle(pApplication->vulkanData.device) != VK_SUCCESS) return EXIT_FAILURE;

	return returnValue;
#endif
}
