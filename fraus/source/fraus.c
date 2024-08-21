#include "../include/fraus/fraus.h"

#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
	#include <windows.h>

	static HMODULE frVulkanLibrary = NULL;
#else
	#include <dlfcn.h>

	static void* frVulkanLibrary = NULL;
#endif

FrResult frCreateApplication(const char* name, uint32_t version)
{
	if(!name || frVulkanLibrary)
	{
		return FR_ERROR_INVALID_ARGUMENT;
	}

#ifdef _WIN32
	frVulkanLibrary = LoadLibrary(TEXT("vulkan-1"));
	if(!frVulkanLibrary)
	{
		return FR_ERROR_FILE_NOT_FOUND;
	}

	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(frVulkanLibrary, "vkGetInstanceProcAddr");
	if(!vkGetInstanceProcAddr)
	{
		return FR_ERROR_UNKNOWN;
	}
#else
#if defined(__APPLE__) || defined(__MACH__)
	frVulkanLibrary = dlopen("libvulkan.dylib", RTLD_LAZY);
#else
	frVulkanLibrary = dlopen("libvulkan.so", RTLD_LAZY);
#endif
	if(!frVulkanLibrary)
	{
		return FR_ERROR_FILE_NOT_FOUND;
	}

	*(void**)(&vkGetInstanceProcAddr) = dlsym(frVulkanLibrary, "vkGetInstanceProcAddr");
	if(!vkGetInstanceProcAddr)
	{
		return FR_ERROR_UNKNOWN;
	}
#endif
	if(frLoadGlobalFunctions() != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	if(frCreateWindow(name) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	if(frCreateVulkanData(name, version) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	frCreateCamera();

	return FR_SUCCESS;
}

/*
 * Clean up Fraus.
 *
 * Returns:
 * - FR_SUCCESS if everything went well
 * - FR_ERROR_UNKNOWN if freeing the Vulkan library failed
 */
FrResult frDestroyApplication(void)
{
	frDestroyWindow();
	if(frDestroyVulkanData() != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

#ifdef _WIN32
	if(!FreeLibrary(frVulkanLibrary))
	{
		return FR_ERROR_UNKNOWN;
	}
#else
	if(dlclose(frVulkanLibrary) != 0)
	{
		return FR_ERROR_UNKNOWN;
	}
#endif

	return FR_SUCCESS;
}

static void frBaseUpdateHandler(float elapsed, void* pUserData)
{
	(void)elapsed;
	(void)pUserData;
}

static FrUpdateHandler updateHandler = frBaseUpdateHandler;
static void* pUpdateHandlerUserData;

void frSetUpdateHandler(FrUpdateHandler handler, void* pUserData)
{
	updateHandler = handler ? handler : frBaseUpdateHandler;
	pUpdateHandlerUserData = pUserData;
}

/*
 * Main loop of the application.
 *
 * Returns:
 * - The exit value of the application.
 */
int frRunApplication(void)
{
	struct timespec lastTime, currentTime;
	if(timespec_get(&lastTime, TIME_UTC) != TIME_UTC)
	{
		return EXIT_FAILURE;
	}

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

		frUpdateGamepad();

		// Update
		if(timespec_get(&currentTime, TIME_UTC) != TIME_UTC)
		{
			return FR_ERROR_UNKNOWN;
		}
		updateHandler(
			((currentTime.tv_sec - lastTime.tv_sec) * UINTMAX_C(1000000000) + currentTime.tv_nsec - lastTime.tv_nsec) / 1000000000.f,
			pUpdateHandlerUserData
		);
		lastTime = currentTime;

		// Render
		if(windowResized)
		{
			GetClientRect(windowHandle, &clientRect);
			if(clientRect.right > 0 && clientRect.bottom > 0)
			{
				if(frRecreateSwapchain() != FR_SUCCESS)
				{
					returnValue = EXIT_FAILURE;
					goto end;
				}

				if(frDrawFrame() != FR_SUCCESS)
				{
					returnValue = EXIT_FAILURE;
					goto end;
				}
			}
		}
		else
		{
			if(frDrawFrame() != FR_SUCCESS)
			{
				returnValue = EXIT_FAILURE;
				goto end;
			}
		}
	}
	end:
#else
	XEvent event;
	while(true)
	{
		while(XQLength(display) > 0)
		{
			XNextEvent(display, &event);
			switch(event.type)
			{
				case ConfigureNotify:
				{
					if(eventHandlers.resizeHandler)
					{
						eventHandlers.resizeHandler(
							event.xconfigure.width,
							event.xconfigure.height,
							eventHandlers.pResizeHandlerUserData
						);
					}

					windowResized = true;

					break;
				}

				case KeyPress:
					if(eventHandlers.keyHandler)
					{
						KeySym keySym = XLookupKeysym(&event.xkey, 0);
						eventHandlers.keyHandler(keySym, FR_KEY_STATE_DOWN, eventHandlers.pKeyHandlerUserData);
					}
					break;

				case KeyRelease:
					if(eventHandlers.keyHandler)
					{
						KeySym keySym = XLookupKeysym(&event.xkey, 0);
						eventHandlers.keyHandler(keySym, FR_KEY_STATE_UP, eventHandlers.pKeyHandlerUserData);
					}
					break;

				case MotionNotify:
					if(eventHandlers.mouseMoveHandler)
					{
						static int previousX;
						static int previousY;

						static bool first = true;

						if(first)
						{
							previousX = event.xmotion.x;
							previousY = event.xmotion.y;

							first = false;
						}

						eventHandlers.mouseMoveHandler(
							event.xmotion.x - previousX,
							event.xmotion.y - previousY,
							eventHandlers.pMouseMoveHandlerUserData
						);

						previousX = event.xmotion.x;
						previousY = event.xmotion.y;
					}
					break;

				default:
					break;
			}
		}

		if(windowResized)
		{
			XWindowAttributes attributes;
			XGetWindowAttributes(display, window, &attributes);

			if(attributes.width > 0 && attributes.height > 0)
			{
				if(frRecreateSwapchain() != FR_SUCCESS)
				{
					return EXIT_FAILURE;
				}
				if(frDrawFrame() != FR_SUCCESS)
				{
					return EXIT_FAILURE;
				}
			}
		}
		else
		{
			if(frDrawFrame() != FR_SUCCESS)
			{
				return EXIT_FAILURE;
			}
		}
	}

	return EXIT_SUCCESS;
#endif
	if(vkDeviceWaitIdle(device) != VK_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	if(frDestroyApplication() != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	return returnValue;
}
