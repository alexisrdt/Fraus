#include "window.h"

#include <stdlib.h>

#ifdef _WIN32

#include <stdbool.h>

static size_t windowCount = 0;

/*
 * Win32 window procedure
 * - window: the window that received a message
 * - message: the message received
 * - wParam: the WPARAM of the message
 * - lParam: the LPARAM of the message
 */
static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_CLOSE:
			DestroyWindow(window);
			break;

		case WM_DESTROY:
			--windowCount;
			if(windowCount == 0) PostQuitMessage(EXIT_SUCCESS);
			break;

		default:
			return DefWindowProc(window, message, wParam, lParam);
	}

	return 0;
}

#endif

/*
 * Create a window
 * - pWindow: pointer to a handle for the window
 */
FrResult frCreateWindow(const wchar_t* pTitle, FrWindow* pWindow)
{
	// Check arguments
	if(!pWindow) return FR_ERROR_INVALID_ARGUMENT;

#ifdef _WIN32
	static HINSTANCE instance = NULL;

	if(!instance)
	{
		instance = GetModuleHandle(NULL);
		if(!instance) return FR_ERROR_UNKNOWN;

		WNDCLASSEX windowClass = {
			.cbSize = sizeof(WNDCLASSEX),
			.lpfnWndProc = WindowProc,
			.hInstance = instance,
			.hIcon = LoadIcon(NULL, IDI_APPLICATION),
			.hCursor = LoadCursor(NULL, IDC_ARROW),
			.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
			.lpszClassName = L"FrWindow",
			.hIcon = LoadIcon(NULL, IDI_APPLICATION)
		};
		if(!RegisterClassExW(&windowClass)) return FR_ERROR_UNKNOWN;
	}

	// TODO: handle specifying title and size (maybe not position, useless)
	*pWindow = CreateWindowExW(
		WS_EX_OVERLAPPEDWINDOW,
		L"FrWindow",
		pTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		instance,
		NULL
	);
	if(!*pWindow) return FR_ERROR_UNKNOWN;
	++windowCount;

	// Show window
	ShowWindow(*pWindow, SW_SHOW);

#endif

	return FR_SUCCESS;
}

/*
 * Main loop of the program
 * - pReturnValue: pointer to a handle for the return value (can be NULL)
 */
void frMainLoop(int* pReturnValue)
{
#ifdef _WIN32

	MSG message;
	while(true)
	{
		while(PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
		{
			// If the message is a quit message, return
			// (also return the quit value if the pointer to handle isn't NULL)
			if(message.message == WM_QUIT)
			{
				if(pReturnValue) *pReturnValue = (int)message.wParam;
				return;
			}

			// Translate and dispatch message to window
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		// Render
	}

#endif
}
