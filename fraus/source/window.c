#include "../include/fraus/window.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
HWND windowHandle;
HINSTANCE windowInstance;
#else
Display* display;
Window window;
#endif
bool windowResized;
bool windowCaptured;
FrEventHandlers eventHandlers;

#ifdef _WIN32
/*
 * Win32 window procedure
 * - handle: the handle of the window that received a message
 * - message: the message received
 * - wParam: the WPARAM of the message
 * - lParam: the LPARAM of the message
 */
static LRESULT CALLBACK WindowProc(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_INPUT:
			if(eventHandlers.mouseMoveHandler)
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
					eventHandlers.mouseMoveHandler(
						input.data.mouse.lLastX,
						input.data.mouse.lLastY,
						eventHandlers.pMouseMoveHandlerUserData
					);
				}

				// Cleanup
				if(GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT)
				{
					DefWindowProc(handle, message, lParam, wParam);
				}

				// Center cursor
				if(windowCaptured)
				{
					RECT windowRect;
					GetWindowRect(handle, &windowRect);

					SetCursorPos(
						windowRect.left + (windowRect.right - windowRect.left) / 2,
						windowRect.top + (windowRect.bottom - windowRect.top) / 2
					);
				}
			}
			break;

		case WM_LBUTTONDOWN:
			if(eventHandlers.keyHandler)
			{
				eventHandlers.keyHandler(FR_KEY_LEFT_MOUSE, FR_KEY_STATE_DOWN, eventHandlers.pKeyHandlerUserData);
			}
			break;

		case WM_LBUTTONUP:
			if(eventHandlers.keyHandler)
			{
				eventHandlers.keyHandler(FR_KEY_LEFT_MOUSE, FR_KEY_STATE_UP, eventHandlers.pKeyHandlerUserData);
			}
			break;

		case WM_RBUTTONDOWN:
			if(eventHandlers.keyHandler)
			{
				eventHandlers.keyHandler(FR_KEY_RIGHT_MOUSE, FR_KEY_STATE_DOWN, eventHandlers.pKeyHandlerUserData);
			}
			break;

		case WM_RBUTTONUP:
			if(eventHandlers.keyHandler)
			{
				eventHandlers.keyHandler(FR_KEY_RIGHT_MOUSE, FR_KEY_STATE_UP, eventHandlers.pKeyHandlerUserData);
			}
			break;

		case WM_KEYDOWN:
			if(eventHandlers.keyHandler)
			{
				eventHandlers.keyHandler(wParam, FR_KEY_STATE_DOWN, eventHandlers.pKeyHandlerUserData);
			}
			break;

		case WM_KEYUP:
			if(eventHandlers.keyHandler)
			{
				eventHandlers.keyHandler(wParam, FR_KEY_STATE_UP, eventHandlers.pKeyHandlerUserData);
			}
			break;

		case WM_SIZE:
		{
			windowResized = true;

			// Get window rect and set cursor position
			if(windowCaptured)
			{
				RECT windowRect;
				GetWindowRect(handle, &windowRect);

				SetCursorPos(
					windowRect.left + (windowRect.right - windowRect.left) / 2,
					windowRect.top + (windowRect.bottom - windowRect.top) / 2
				);
			}

			if(eventHandlers.resizeHandler)
			{
				eventHandlers.resizeHandler(LOWORD(lParam), HIWORD(lParam), eventHandlers.pResizeHandlerUserData);
			}

			break;
		}

		case WM_CLOSE:
			DestroyWindow(handle);
			break;

		case WM_DESTROY:
			PostQuitMessage(EXIT_SUCCESS);
			break;

		default:
			return DefWindowProc(handle, message, wParam, lParam);
	}

	return 0;
}
#endif

/*
 * Create a window
 * - pTitle: the title of the window
 * - pWindow: pointer to a handle for the window
 */
FrResult frCreateWindow(const char* title)
{
	windowResized = false;
	windowCaptured = false;
	memset(&eventHandlers, 0, sizeof(eventHandlers));

#ifdef _WIN32
	windowInstance = GetModuleHandle(NULL);
	if(!windowInstance)
	{
		return FR_ERROR_UNKNOWN;
	}

	const WNDCLASSEX windowClass = {
		.cbSize = sizeof(windowClass),
		.lpfnWndProc = WindowProc,
		.hInstance = windowInstance,
		.hIcon = LoadIcon(NULL, IDI_APPLICATION),
		.hCursor = LoadCursor(NULL, IDC_ARROW),
		.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
		.lpszClassName = TEXT("FrWindow"),
		.hIconSm = LoadIcon(NULL, IDI_APPLICATION)
	};
	if(!RegisterClassEx(&windowClass))
	{
		return FR_ERROR_UNKNOWN;
	}

#if defined(UNICODE) || defined(_UNICODE)
	const int wideTitleSize = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
	if(!wideTitleSize)
	{
		return FR_ERROR_UNKNOWN;
	}

	WCHAR* const wideTitle = malloc(wideTitleSize * sizeof(wideTitle[0]));
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

	windowHandle = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		TEXT("FrWindow"),
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
		NULL,
		NULL,
		windowInstance,
		NULL
	);
#if defined(UNICODE) || defined(_UNICODE)
	free(wideTitle);
#endif
	if(!windowHandle)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Show window
	ShowWindow(windowHandle, SW_SHOW);

	// Register devices
	const RAWINPUTDEVICE mouseDevice = {
		.usUsagePage = 1,
		.usUsage = 2,
		.hwndTarget = windowHandle
	};
	if(RegisterRawInputDevices(&mouseDevice, 1, sizeof(mouseDevice)) != TRUE)
	{
		return FR_ERROR_UNKNOWN;
	}
#else
	// Open display
	display = XOpenDisplay(NULL);
	if(!display)
	{
		return FR_ERROR_UNKNOWN;
	}

	// Create window
	int screen = DefaultScreen(display);
	window = XCreateSimpleWindow(
		display,
		RootWindow(display, screen),
		0, 0,
		640, 360,
		0,
		BlackPixel(display, screen),
		WhitePixel(display, screen)
	);
	XStoreName(display, window, pTitle);
	XSelectInput(display, window, KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);

	// Show window
	XMapWindow(display, window);
#endif

	return FR_SUCCESS;
}

/*
 * Destroy a window
 * - pWindow: pointer to the window
 */
void frDestroyWindow(void)
{
#ifdef _WIN32
	DestroyWindow(windowHandle);
#else
	XDestroyWindow(pWindow->display, pWindow->window);
	XCloseDisplay(pWindow->display);
#endif
}

void frCloseWindow(void)
{
#ifdef _WIN32
	PostMessage(windowHandle, WM_CLOSE, 0, 0);
#else
	XEvent event;
	event.type = ClientMessage;
	event.xclient.window = pWindow->window;
	event.xclient.message_type = XInternAtom(pWindow->display, "WM_PROTOCOLS", False);
	event.xclient.format = 32;
	event.xclient.data.l[0] = XInternAtom(pWindow->display, "WM_DELETE_WINDOW", False);
	event.xclient.data.l[1] = CurrentTime;
	XSendEvent(pWindow->display, pWindow->window, False, NoEventMask, &event);
#endif
}

void frMaximizeWindow(void)
{
	WINDOWPLACEMENT windowPlacement;
	GetWindowPlacement(windowHandle, &windowPlacement);

	ShowWindow(windowHandle, windowPlacement.showCmd == SW_MAXIMIZE ? SW_RESTORE : SW_MAXIMIZE);
}

void frCaptureMouse(bool capture)
{
	windowCaptured = capture;

#ifdef _WIN32
	if(capture)
	{
		RECT windowRect;
		GetWindowRect(windowHandle, &windowRect);

		SetCursorPos(
			windowRect.left + (windowRect.right - windowRect.left) / 2,
			windowRect.top + (windowRect.bottom - windowRect.top) / 2
		);
	}

	ShowCursor(!capture);
#else
	if(capture)
	{
		XWindowAttributes attributes;
		XGetWindowAttributes(pWindow->display, pWindow->window, &attributes);

		XWarpPointer(pWindow->display, None, pWindow->window, 0, 0, 0, 0, attributes.width / 2, attributes.height / 2);
	}
#endif
}

/*
 * Get the state of a key
 * - key: the key
 */
FrKeyState frGetKeyState(FrKey key)
{
#ifdef _WIN32
	return GetKeyState(key) >= 0 ? FR_KEY_STATE_UP : FR_KEY_STATE_DOWN;
#else
	char keyboardState[32];
	XQueryKeymap(display, keyboardState);

	return keyboardState[key >> 3] & (1 << (key & 7));
#endif
}

/* Handlers */

void frSetMouseMoveHandler(FrMouseMoveHandler handler, void* pUserData)
{
	eventHandlers.mouseMoveHandler = handler;
	eventHandlers.pMouseMoveHandlerUserData = pUserData;
}

void frSetKeyHandler(FrKeyHandler handler, void* pUserData)
{
	eventHandlers.keyHandler = handler;
	eventHandlers.pKeyHandlerUserData = pUserData;
}

void frSetResizeHandler(FrResizeHandler handler, void* pUserData)
{
	eventHandlers.resizeHandler = handler;
	eventHandlers.pResizeHandlerUserData = pUserData;
}
