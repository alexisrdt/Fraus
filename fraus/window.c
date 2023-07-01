#include "window.h"

#include <stdlib.h>

#ifdef _WIN32

#include <stdbool.h>

// Window count
static size_t windowCount = 0;

/*
 * Translate a Win32 virtual-key code to a FrKey
 * - virtualKey: the Win32 virtual-key
 */
static FrKey frWin32VirtualKeyToFrKey(WPARAM virtualKey)
{
	switch(virtualKey)
	{
		case 0x41: return FR_KEY_A;
		case 0x42: return FR_KEY_B;
		case 0x43: return FR_KEY_C;
		case 0x44: return FR_KEY_D;
		case 0x45: return FR_KEY_E;
		case 0x46: return FR_KEY_F;
		case 0x47: return FR_KEY_G;
		case 0x48: return FR_KEY_H;
		case 0x49: return FR_KEY_I;
		case 0x4A: return FR_KEY_J;
		case 0x4B: return FR_KEY_K;
		case 0x4C: return FR_KEY_L;
		case 0x4D: return FR_KEY_M;
		case 0x4E: return FR_KEY_N;
		case 0x4F: return FR_KEY_O;
		case 0x50: return FR_KEY_P;
		case 0x51: return FR_KEY_Q;
		case 0x52: return FR_KEY_R;
		case 0x53: return FR_KEY_S;
		case 0x54: return FR_KEY_T;
		case 0x55: return FR_KEY_U;
		case 0x56: return FR_KEY_V;
		case 0x57: return FR_KEY_W;
		case 0x58: return FR_KEY_X;
		case 0x59: return FR_KEY_Y;
		case 0x5A: return FR_KEY_Z;

		default: return FR_KEY_UNKNOWN;
	}
}

/*
 * Win32 window procedure
 * - window: the window that received a message
 * - message: the message received
 * - wParam: the WPARAM of the message
 * - lParam: the LPARAM of the message
 */
static LRESULT CALLBACK WindowProc(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
	FrWindow* pWindow = (FrWindow*)GetProp(handle, TEXT("FrWindow"));

	switch(message)
	{
		case WM_LBUTTONDOWN:
			if(pWindow->handlers.clickHandler) pWindow->handlers.clickHandler(FR_MOUSE_LEFT);
			break;

		case WM_RBUTTONDOWN:
			if(pWindow->handlers.clickHandler) pWindow->handlers.clickHandler(FR_MOUSE_RIGHT);
			break;

		case WM_KEYDOWN:
			if(pWindow->handlers.keyHandler) pWindow->handlers.keyHandler(frWin32VirtualKeyToFrKey(wParam));
			break;

		case WM_CLOSE:
			DestroyWindow(handle);
			break;

		case WM_DESTROY:
			--windowCount;
			if(windowCount == 0) PostQuitMessage(EXIT_SUCCESS);
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
	pWindow->handle = CreateWindowExW(
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
		pWindow
	);
	if(!pWindow->handle) return FR_ERROR_UNKNOWN;
	++windowCount;

	// Setup window data
	pWindow->handlers.clickHandler = NULL;
	pWindow->handlers.keyHandler = NULL;
	SetProp(pWindow->handle, TEXT("FrWindow"), (HANDLE)pWindow);

	// Show window
	ShowWindow(pWindow->handle, SW_SHOW);

#endif

	return FR_SUCCESS;
}

/*
 * Set the click handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetClickHandler(FrWindow* pWindow, FrClickHandler handler)
{
	pWindow->handlers.clickHandler = handler;
}

/*
 * Set the key handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetKeyHandler(FrWindow* pWindow, FrKeyHandler handler)
{
	pWindow->handlers.keyHandler = handler;
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
