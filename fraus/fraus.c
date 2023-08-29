#include "fraus.h"

#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#ifndef NDEBUG
#include <crtdbg.h>
#endif

static HMODULE vulkanLibrary;
#endif

FrResult frInit(void)
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

FrResult frFinish(void)
{

#ifdef _WIN32

	if(!FreeLibrary(vulkanLibrary)) return FR_ERROR_UNKNOWN;

#ifndef NDEBUG
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);

	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);

	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

	_CrtDumpMemoryLeaks();
#endif

#endif

	return FR_SUCCESS;
}

/*
 * Main loop of the program
 * - pVulkanData: pointer to a the Vulkan Data
 */
#include <stdio.h>
int frMainLoop(FrVulkanData* pVulkanData)
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
		if(pVulkanData->window.resized)
		{
			GetClientRect(pVulkanData->window.handle, &clientRect);
			if(clientRect.right > 0 && clientRect.bottom > 0)
			{
				if(frRecreateSwapchain(pVulkanData) != FR_SUCCESS)
				{
					returnValue = EXIT_FAILURE;
					goto end;
				}

				if(frDrawFrame(pVulkanData) != FR_SUCCESS)
				{
					returnValue = EXIT_FAILURE;
					goto end;
				}
			}
		}
		else
		{
			if(frDrawFrame(pVulkanData) != FR_SUCCESS)
			{
				returnValue = EXIT_FAILURE;
				goto end;
			}
		}
	}
	end:

	if(vkDeviceWaitIdle(pVulkanData->device) != VK_SUCCESS) return EXIT_FAILURE;

	return returnValue;

#endif
}
