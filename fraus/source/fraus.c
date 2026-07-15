#include "../include/fraus/fraus.h"

#include <stdlib.h>
#include <time.h>

#include <windows.h>

FrResult frInitialize(void)
{
	windowInstance = GetModuleHandle(nullptr);
	if(!windowInstance)
	{
		return FR_ERROR_UNKNOWN;
	}

	const FrResult result = frInitializeWindow();

	frInitializeVulkan();

	return result;
}

FrResult frFinish(void)
{
	return frFinishVulkan();
}

static void frBaseUpdateHandler(const float elapsed, void* const userData)
{
	(void)elapsed;
	(void)userData;
}

FrResult frCreateApplication(FrApplication* const application, const char* const name, const uint32_t version)
{
	application->mouseMoveHandler = nullptr;
	application->keyHandler = nullptr;
	application->resizeHandler = nullptr;
	application->updateHandler = frBaseUpdateHandler;

	frArenaCreate(&application->arena);

	if(frCreateWindow(application, &application->window, name) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	if(frCreateVulkanEngine(application, name, version) != FR_SUCCESS)
	{
		return FR_ERROR_UNKNOWN;
	}

	frCreateCamera(&application->camera);

	return FR_SUCCESS;
}

/*
 * Clean up Fraus.
 *
 * Returns:
 * - FR_SUCCESS if everything went well
 * - FR_ERROR_UNKNOWN if freeing the Vulkan library failed
 */
FrResult frDestroyApplication(FrApplication* const application)
{
	frDestroyWindow(&application->window);
	frDestroyVulkanEngine(&application->engine);

	return FR_SUCCESS;
}

void frSetMouseMoveHandler(FrApplication* const application, const FrMouseMoveHandler handler, void* const userData)
{
	application->mouseMoveHandler = handler;
	application->mouseMoveHandlerUserData = userData;
}

void frSetKeyHandler(FrApplication* const application, const FrKeyHandler handler, void* const userData)
{
	application->keyHandler = handler;
	application->keyHandlerUserData = userData;
}

void frSetResizeHandler(FrApplication* const application, const FrResizeHandler handler, void* const userData)
{
	application->resizeHandler = handler;
	application->resizeHandlerUserData = userData;
}

void frSetUpdateHandler(FrApplication* const application, const FrUpdateHandler handler, void* const userData)
{
	application->updateHandler = handler;
	application->updateHandlerUserData = userData;
}

/*
 * Main loop of the application.
 *
 * Returns:
 * - The exit value of the application.
 */
int frRunApplication(FrApplication* const application)
{
	struct timespec lastTime, currentTime;
	if(timespec_get(&lastTime, TIME_UTC) != TIME_UTC)
	{
		return EXIT_FAILURE;
	}

	int returnValue;
	RECT clientRect;
	MSG message;
	while(true)
	{
		// Handle events
		while(PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
		{
			// If the message is a quit message, return
			// (also return the quit value if the pointer to handle isn't nullptr)
			if(message.message == WM_QUIT)
			{
				returnValue = (int)message.wParam;
				goto end;
			}

			// Translate and dispatch message to window
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		// Update
		if(timespec_get(&currentTime, TIME_UTC) != TIME_UTC)
		{
			return FR_ERROR_UNKNOWN;
		}
		application->updateHandler(
			((currentTime.tv_sec - lastTime.tv_sec) * UINTMAX_C(1000000000) + currentTime.tv_nsec - lastTime.tv_nsec) / 1000000000.f,
			application->updateHandlerUserData
		);
		lastTime = currentTime;

		// Render
		if(application->window.resized)
		{
			GetClientRect(application->window.window, &clientRect);
			if(clientRect.right > 0 && clientRect.bottom > 0)
			{
				if(frRecreateSwapchain(application) != FR_SUCCESS)
				{
					returnValue = EXIT_FAILURE;
					goto end;
				}

				if(frDrawFrame(application) != FR_SUCCESS)
				{
					returnValue = EXIT_FAILURE;
					goto end;
				}
			}
		}
		else
		{
			if(frDrawFrame(application) != FR_SUCCESS)
			{
				returnValue = EXIT_FAILURE;
				goto end;
			}
		}
	}
	end:

	if(application->engine.vkDeviceWaitIdle(application->engine.device) != VK_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	if(frDestroyApplication(application) != FR_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	return returnValue;
}
