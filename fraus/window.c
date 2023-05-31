#include "window.h"

#include <stdlib.h>

#ifdef _WIN32

#include <stdbool.h>

static HINSTANCE instance = NULL;

static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_CLOSE:
			DestroyWindow(window); // TODO: check return value
			break;

		case WM_DESTROY:
			PostQuitMessage(EXIT_SUCCESS); // TODO: handle multiple windows
			break;

		default:
			return DefWindowProc(window, message, wParam, lParam);
	}

	return 0;
}

#endif

FrResult frCreateWindow(FrWindow* pWindow)
{
#ifdef _WIN32

	if(!instance)
	{
		instance = GetModuleHandle(NULL);
		if(!instance) return FR_ERROR_UNKNOWN; // TODO: check doc

		WNDCLASSEX window_class = {
			.cbSize = sizeof(WNDCLASSEX),
			.lpfnWndProc = WindowProc,
			.hInstance = instance,
			.hIcon = LoadIcon(NULL, IDI_APPLICATION),
			.hCursor = LoadCursor(NULL, IDC_ARROW),
			.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
			.lpszClassName = TEXT("FrWindow"),
			.hIcon = LoadIcon(NULL, IDI_APPLICATION)
		};
		if(!RegisterClassEx(&window_class)) return FR_ERROR_UNKNOWN;
	}

	// TODO: handle specifying title and size (maybe not position, useless)
	*pWindow = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		TEXT("FrWindow"),
		TEXT("Fraus application"),
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

	ShowWindow(*pWindow, SW_SHOW);

#endif

	return FR_SUCCESS;
}

void frMainLoop(int* pReturnValue)
{
#ifdef _WIN32

	MSG message;
	while(true)
	{
		int i = 0;

		while(PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
		{
			++i;

			if(message.message == WM_QUIT)
			{
				if(pReturnValue) *pReturnValue = (int)message.wParam;
				return;
			}

			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		// Render
	}

#endif
}
