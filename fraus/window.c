#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
		case WM_MOUSEMOVE:
			if(pWindow->handlers.mouseMoveHandler) pWindow->handlers.mouseMoveHandler(LOWORD(lParam), HIWORD(lParam), pWindow->handlers.pMouseMoveHandlerUserData);
			break;

		case WM_LBUTTONDOWN:
			if(pWindow->handlers.clickHandler) pWindow->handlers.clickHandler(FR_MOUSE_LEFT, pWindow->handlers.pClickHandlerUserData);
			break;

		case WM_RBUTTONDOWN:
			if(pWindow->handlers.clickHandler) pWindow->handlers.clickHandler(FR_MOUSE_RIGHT, pWindow->handlers.pClickHandlerUserData);
			break;

		case WM_KEYDOWN:
			if(pWindow->handlers.keyHandler) pWindow->handlers.keyHandler(frWin32VirtualKeyToFrKey(wParam), pWindow->handlers.pKeyHandlerUserData);
			break;

		case WM_SIZE:
			if(pWindow->handlers.resizeHandler) pWindow->handlers.resizeHandler(LOWORD(lParam), HIWORD(lParam), pWindow->handlers.pResizeHandlerUserData);
			break;

		case WM_CLOSE:
			RemoveProp(handle, TEXT("FrWindow"));
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
	memset(&pWindow->handlers, 0, sizeof(FrEventHandlers));
	SetProp(pWindow->handle, TEXT("FrWindow"), (HANDLE)pWindow);

	// Show window
	ShowWindow(pWindow->handle, SW_SHOW);
#endif

	return FR_SUCCESS;
}

/* Handlers */

/*
 * Set the mouse move handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetMouseMoveHandler(FrWindow* pWindow, FrMouseMoveHandler handler, void* pUserData)
{
	pWindow->handlers.mouseMoveHandler = handler;
	pWindow->handlers.pMouseMoveHandlerUserData = pUserData;
}

/*
 * Set the click handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetClickHandler(FrWindow* pWindow, FrClickHandler handler, void* pUserData)
{
	pWindow->handlers.clickHandler = handler;
	pWindow->handlers.pClickHandlerUserData = pUserData;
}

/*
 * Set the key handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetKeyHandler(FrWindow* pWindow, FrKeyHandler handler, void* pUserData)
{
	pWindow->handlers.keyHandler = handler;
	pWindow->handlers.pKeyHandlerUserData = pUserData;
}

/*
 * Set the resize handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetResizeHandler(FrWindow* pWindow, FrResizeHandler handler, void* pUserData)
{
	pWindow->handlers.resizeHandler = handler;
	pWindow->handlers.pResizeHandlerUserData = pUserData;
}
