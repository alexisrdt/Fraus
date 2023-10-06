#include "window.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

// Window count
static size_t windowCount = 0;

/*
 * Translate a Win32 virtual-key code to an FrKey
 * - virtualKey: the Win32 virtual-key
 */
static FrKey frWin32VirtualKeyToFrKey(WPARAM virtualKey)
{
	switch(virtualKey)
	{
		case VK_LBUTTON: return FR_KEY_LEFT_MOUSE;
		case VK_RBUTTON: return FR_KEY_RIGHT_MOUSE;

		case 'A': return FR_KEY_A;
		case 'B': return FR_KEY_B;
		case 'C': return FR_KEY_C;
		case 'D': return FR_KEY_D;
		case 'E': return FR_KEY_E;
		case 'F': return FR_KEY_F;
		case 'G': return FR_KEY_G;
		case 'H': return FR_KEY_H;
		case 'I': return FR_KEY_I;
		case 'J': return FR_KEY_J;
		case 'K': return FR_KEY_K;
		case 'L': return FR_KEY_L;
		case 'M': return FR_KEY_M;
		case 'N': return FR_KEY_N;
		case 'O': return FR_KEY_O;
		case 'P': return FR_KEY_P;
		case 'Q': return FR_KEY_Q;
		case 'R': return FR_KEY_R;
		case 'S': return FR_KEY_S;
		case 'T': return FR_KEY_T;
		case 'U': return FR_KEY_U;
		case 'V': return FR_KEY_V;
		case 'W': return FR_KEY_W;
		case 'X': return FR_KEY_X;
		case 'Y': return FR_KEY_Y;
		case 'Z': return FR_KEY_Z;

		case VK_LEFT: return FR_KEY_LEFT;
		case VK_RIGHT: return FR_KEY_RIGHT;
		case VK_UP: return FR_KEY_UP;
		case VK_DOWN: return FR_KEY_DOWN;

		case VK_SPACE: return FR_KEY_SPACE;

		case VK_LCONTROL: return FR_KEY_LEFT_CONTROL;
		case VK_RCONTROL: return FR_KEY_RIGHT_CONTROL;
		case VK_LSHIFT: return FR_KEY_LEFT_SHIFT;
		case VK_RSHIFT: return FR_KEY_RIGHT_SHIFT;

		case VK_ESCAPE: return FR_KEY_ESCAPE;

		default: return FR_KEY_UNKNOWN;
	}
}

/*
 * Translate an FrKeyto a Win32 virtual-key code
 * - key: the FrKey
 */
static int frFrKeyToWin32VirtualKey(FrKey key)
{
	switch(key)
	{
		case FR_KEY_LEFT_MOUSE: return VK_LBUTTON;
		case FR_KEY_RIGHT_MOUSE: return VK_RBUTTON;

		case FR_KEY_A: return 'A';
		case FR_KEY_B: return 'B';
		case FR_KEY_C: return 'C';
		case FR_KEY_D: return 'D';
		case FR_KEY_E: return 'E';
		case FR_KEY_F: return 'F';
		case FR_KEY_G: return 'G';
		case FR_KEY_H: return 'H';
		case FR_KEY_I: return 'I';
		case FR_KEY_J: return 'J';
		case FR_KEY_K: return 'K';
		case FR_KEY_L: return 'L';
		case FR_KEY_M: return 'M';
		case FR_KEY_N: return 'N';
		case FR_KEY_O: return 'O';
		case FR_KEY_P: return 'P';
		case FR_KEY_Q: return 'Q';
		case FR_KEY_R: return 'R';
		case FR_KEY_S: return 'S';
		case FR_KEY_T: return 'T';
		case FR_KEY_U: return 'U';
		case FR_KEY_V: return 'V';
		case FR_KEY_W: return 'W';
		case FR_KEY_X: return 'X';
		case FR_KEY_Y: return 'Y';
		case FR_KEY_Z: return 'Z';

		case FR_KEY_LEFT: return VK_LEFT;
		case FR_KEY_RIGHT: return VK_RIGHT;
		case FR_KEY_UP: return VK_UP;
		case FR_KEY_DOWN: return VK_DOWN;

		case FR_KEY_SPACE: return VK_SPACE;

		case FR_KEY_LEFT_CONTROL: return VK_LCONTROL;
		case FR_KEY_RIGHT_CONTROL: return VK_RCONTROL;
		case FR_KEY_LEFT_SHIFT: return VK_LSHIFT;
		case FR_KEY_RIGHT_SHIFT: return VK_RSHIFT;

		case FR_KEY_ESCAPE: return VK_ESCAPE;

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
		case WM_CREATE:
		{
			// Register mouse device
			RAWINPUTDEVICE device = {
				.usUsagePage = 1,
				.usUsage = 2,
				.dwFlags = 0,
				.hwndTarget = handle
			};
			RegisterRawInputDevices(&device, 1, sizeof(RAWINPUTDEVICE));

			break;
		}

		case WM_SETFOCUS:
			// Hide cursor
			if(pWindow->capture) ShowCursor(false);
			break;

		case WM_KILLFOCUS:
			// Show cursor
			if(pWindow->capture) ShowCursor(true);
			break;

		case WM_INPUT:
			if(pWindow->handlers.mouseMoveHandler)
			{
				static UINT size = sizeof(RAWINPUT);
				static RAWINPUT input;

				GetRawInputData(
					(HRAWINPUT)lParam,
					RID_INPUT,
					&input,
					&size,
					sizeof(RAWINPUTHEADER)
				);

				if(input.header.dwType == RIM_TYPEMOUSE && input.data.mouse.usFlags == MOUSE_MOVE_RELATIVE)
				{
					pWindow->handlers.mouseMoveHandler(
						input.data.mouse.lLastX,
						input.data.mouse.lLastY,
						pWindow->handlers.pMouseMoveHandlerUserData
					);
				}

				// Cleanup
				if(GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT)
				{
					DefWindowProc(handle, message, lParam, wParam);
				}

				// Center cursor
				if(pWindow->capture)
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
			if(pWindow->handlers.keyHandler) pWindow->handlers.keyHandler(FR_KEY_LEFT_MOUSE, FR_KEY_STATE_DOWN, pWindow->handlers.pKeyHandlerUserData);
			break;

		case WM_LBUTTONUP:
			if(pWindow->handlers.keyHandler) pWindow->handlers.keyHandler(FR_KEY_LEFT_MOUSE, FR_KEY_STATE_UP, pWindow->handlers.pKeyHandlerUserData);
			break;

		case WM_RBUTTONDOWN:
			if(pWindow->handlers.keyHandler) pWindow->handlers.keyHandler(FR_KEY_RIGHT_MOUSE, FR_KEY_STATE_DOWN, pWindow->handlers.pKeyHandlerUserData);
			break;

		case WM_RBUTTONUP:
			if(pWindow->handlers.keyHandler) pWindow->handlers.keyHandler(FR_KEY_RIGHT_MOUSE, FR_KEY_STATE_UP, pWindow->handlers.pKeyHandlerUserData);
			break;

		case WM_KEYDOWN:
			if(pWindow->handlers.keyHandler) pWindow->handlers.keyHandler(frWin32VirtualKeyToFrKey(wParam), FR_KEY_STATE_DOWN, pWindow->handlers.pKeyHandlerUserData);
			break;

		case WM_KEYUP:
			if (pWindow->handlers.keyHandler) pWindow->handlers.keyHandler(frWin32VirtualKeyToFrKey(wParam), FR_KEY_STATE_UP, pWindow->handlers.pKeyHandlerUserData);
			break;

		case WM_SIZE:
		{
			pWindow->resized = true;

			// Get window rect and set cursor position
			if(pWindow->capture)
			{
				RECT windowRect;
				GetWindowRect(handle, &windowRect);

				SetCursorPos(
					windowRect.left + (windowRect.right - windowRect.left) / 2,
					windowRect.top + (windowRect.bottom - windowRect.top) / 2
				);
			}

			if(pWindow->handlers.resizeHandler) pWindow->handlers.resizeHandler(LOWORD(lParam), HIWORD(lParam), pWindow->handlers.pResizeHandlerUserData);

			break;
		}

		case WM_CLOSE:
			DestroyWindow(handle);
			break;

		case WM_DESTROY:
			RemoveProp(handle, TEXT("FrWindow"));

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
FrResult frCreateWindow(const char* pTitle, FrWindow* pWindow)
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
			.lpszClassName = TEXT("FrWindow"),
			.hIcon = LoadIcon(NULL, IDI_APPLICATION)
		};
		if(!RegisterClassEx(&windowClass)) return FR_ERROR_UNKNOWN;
	}

#if defined(UNICODE) || defined(_UNICODE)
	int wideTitleSize = MultiByteToWideChar(CP_UTF8, 0, pTitle, -1, NULL, 0);
	if(!wideTitleSize) return FR_ERROR_UNKNOWN;

	WCHAR* pWideTitle = malloc(wideTitleSize * sizeof(WCHAR));
	if(!pWideTitle) return FR_ERROR_OUT_OF_MEMORY;

	if(MultiByteToWideChar(CP_UTF8, 0, pTitle, -1, pWideTitle, wideTitleSize) != wideTitleSize)
	{
		free(pWideTitle);
		return FR_ERROR_UNKNOWN;
	}
#endif

	pWindow->handle = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		TEXT("FrWindow"),
	#if defined(UNICODE) || defined(_UNICODE)
		pWideTitle,
	#else
		pTitle,
	#endif
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
#if defined(UNICODE) || defined(_UNICODE)
	free(pWideTitle);
#endif
	if(!pWindow->handle) return FR_ERROR_UNKNOWN;
	++windowCount;

	// Setup window data
	pWindow->resized = false;
	pWindow->capture = false;
	memset(&pWindow->handlers, 0, sizeof(FrEventHandlers));
	SetProp(pWindow->handle, TEXT("FrWindow"), (HANDLE)pWindow);

	// Show window
	ShowWindow(pWindow->handle, SW_SHOW);
#endif

	return FR_SUCCESS;
}

/*
 * Destroy a window
 * - pWindow: pointer to the window
 */
void frDestroyWindow(FrWindow* pWindow)
{
#ifdef _WIN32
	DestroyWindow(pWindow->handle);
#endif
}

/*
 * Capture the mouse
 * - pWindow: pointer to the window
 * - capture: true to capture the mouse, false to release it
 */
void frCaptureMouse(FrWindow* pWindow, bool capture)
{
	pWindow->capture = capture;

	if(capture)
	{
		RECT windowRect;
		GetWindowRect(pWindow->handle, &windowRect);

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
FrKeyState frGetKeyState(FrKey key)
{
#ifdef _WIN32
	return GetKeyState(frFrKeyToWin32VirtualKey(key)) >= 0 ? FR_KEY_STATE_UP : FR_KEY_STATE_DOWN;
#endif
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
