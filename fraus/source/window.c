#include "../include/fraus/window.h"

#include <stdlib.h>
#include <string.h>

#include "fraus/fraus.h"

HINSTANCE windowInstance;

/*
 * Win32 window procedure
 * - handle: the handle of the window that received a message
 * - message: the message received
 * - wParam: the WPARAM of the message
 * - lParam: the LPARAM of the message
 */
static LRESULT CALLBACK WindowProc(const HWND window, const UINT message, const WPARAM wParam, const LPARAM lParam)
{
	FrApplication* const application = (void*)GetWindowLongPtr(window, GWLP_USERDATA);

	switch(message)
	{
		case WM_INPUT:
			if(application->mouseMoveHandler)
			{
				RAWINPUT input;
				UINT size = sizeof(input);
				GetRawInputData(
					(HRAWINPUT)lParam,
					RID_INPUT,
					&input,
					&size,
					sizeof(input.header)
				);

				if(input.header.dwType == RIM_TYPEMOUSE && input.data.mouse.usFlags == MOUSE_MOVE_RELATIVE)
				{
					application->mouseMoveHandler(
						input.data.mouse.lLastX,
						input.data.mouse.lLastY,
						application->mouseMoveHandlerUserData
					);
				}

				// Cleanup
				if(GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT)
				{
					DefWindowProc(window, message, lParam, wParam);
				}

				// Center cursor
				if(application->window.captured)
				{
					RECT windowRect;
					GetWindowRect(window, &windowRect);

					SetCursorPos(
						windowRect.left + (windowRect.right - windowRect.left) / 2,
						windowRect.top + (windowRect.bottom - windowRect.top) / 2
					);
				}
			}
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			if(application->keyHandler)
			{
				const bool isLeft = message == WM_LBUTTONDOWN || message == WM_LBUTTONUP;
				const bool isDown = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN;

				application->keyHandler(isLeft ? FR_KEY_LEFT_MOUSE : FR_KEY_RIGHT_MOUSE, isDown ? FR_KEY_STATE_DOWN : FR_KEY_STATE_UP, application->keyHandlerUserData);
			}
			break;

		case WM_KEYDOWN:
			if(application->keyHandler)
			{
				application->keyHandler(wParam, message == WM_KEYDOWN ? FR_KEY_STATE_DOWN : FR_KEY_STATE_UP, application->keyHandlerUserData);
			}
			break;

		case WM_SIZE:
			if(application)
			{
				application->window.resized = true;

				// Get window rect and set cursor position
				if(application->window.captured)
				{
					RECT windowRect;
					GetWindowRect(window, &windowRect);

					SetCursorPos(
						windowRect.left + (windowRect.right - windowRect.left) / 2,
						windowRect.top + (windowRect.bottom - windowRect.top) / 2
					);
				}

				if(application->resizeHandler)
				{
					application->resizeHandler(LOWORD(lParam), HIWORD(lParam), application->resizeHandlerUserData);
				}
			}
			break;

		case WM_CLOSE:
			DestroyWindow(window);
			break;

		case WM_DESTROY:
			PostQuitMessage(EXIT_SUCCESS);
			break;

		default:
			return DefWindowProc(window, message, wParam, lParam);
	}

	return 0;
}

static ATOM windowClass;

FrResult frInitializeWindow(void)
{
	const WNDCLASSEX windowClassInfo = {
		.cbSize = sizeof(windowClassInfo),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = WindowProc,
		.hInstance = windowInstance,
		.hCursor = LoadCursor(nullptr, IDC_ARROW),
		.hbrBackground = GetStockObject(BLACK_BRUSH),
		.lpszClassName = TEXT("FrWindow"),
	};
	windowClass = RegisterClassEx(&windowClassInfo);
	if(!windowClass)
	{
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

/*
 * Create a window
 * - title: the title of the window
 */
FrResult frCreateWindow(FrApplication* const application, FrWindow* const window, const char* const title)
{
	window->resized = false;
	window->captured = false;

#if defined(UNICODE) || defined(_UNICODE)
	const int wideTitleSize = MultiByteToWideChar(CP_UTF8, 0, title, -1, nullptr, 0);
	if(!wideTitleSize)
	{
		return FR_ERROR_UNKNOWN;
	}

	const FrArenaSave save = frArenaSave(&application->arena);
	WCHAR* const wideTitle = frArenaAllocate(&application->arena, wideTitleSize * sizeof(wideTitle[0]), alignof(typeof(wideTitle[0])));
	if(!wideTitle)
	{
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	if(MultiByteToWideChar(CP_UTF8, 0, title, -1, wideTitle, wideTitleSize) != wideTitleSize)
	{
		free(wideTitle);
		return FR_ERROR_UNKNOWN;
	}
#endif

	window->window = CreateWindowEx(
		0,
		MAKEINTATOM(windowClass),
	#if defined(UNICODE) || defined(_UNICODE)
		wideTitle,
	#else
		title,
	#endif
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		nullptr,
		nullptr,
		windowInstance,
		nullptr
	);
#if defined(UNICODE) || defined(_UNICODE)
	frArenaRestore(&application->arena, save);
#endif
	if(!window->window)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Show window
	ShowWindow(window->window, SW_SHOW);

	SetWindowLongPtr(window->window, GWLP_USERDATA, (LONG_PTR)(void*)application);

	// Register devices
	const RAWINPUTDEVICE mouseDevice = {
		.usUsagePage = 1,
		.usUsage = 2,
		.hwndTarget = window->window
	};
	if(RegisterRawInputDevices(&mouseDevice, 1, sizeof(mouseDevice)) != TRUE)
	{
		return FR_ERROR_UNKNOWN;
	}

	return FR_SUCCESS;
}

/*
 * Destroy a window
 */
void frDestroyWindow(FrWindow* const window)
{
	DestroyWindow(window->window);
}

void frCloseWindow(FrWindow* const window)
{
	PostMessage(window->window, WM_CLOSE, 0, 0);
}

void frMaximizeWindow(FrWindow* const window)
{
	WINDOWPLACEMENT windowPlacement;
	GetWindowPlacement(window->window, &windowPlacement);

	ShowWindow(window->window, windowPlacement.showCmd == SW_MAXIMIZE ? SW_RESTORE : SW_MAXIMIZE);
}

void frCaptureMouse(FrWindow* const window, const bool capture)
{
	window->captured = capture;

	if(capture)
	{
		RECT windowRect;
		GetWindowRect(window->window, &windowRect);

		SetCursorPos(
			windowRect.left + (windowRect.right - windowRect.left) / 2,
			windowRect.top + (windowRect.bottom - windowRect.top) / 2
		);
	}

	ShowCursor(!capture);
}

/*
 * Get the state of a key
 * - key: the key
 */
FrKeyState frGetKeyState(const FrKey key)
{
	return GetKeyState(key) >= 0 ? FR_KEY_STATE_UP : FR_KEY_STATE_DOWN;
}
