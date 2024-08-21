#ifndef FRAUS_WINDOW_H
#define FRAUS_WINDOW_H

#include <stdbool.h>
#include <stdint.h>

#include "utils.h"

#ifdef _WIN32
	#include <windows.h>
#else
	#include <X11/Xlib.h>
#endif

// Keyboard key enum
typedef enum FrKey
{
	#ifdef _WIN32
	FR_KEY_LEFT_MOUSE = VK_LBUTTON,
	FR_KEY_RIGHT_MOUSE = VK_RBUTTON,

	FR_KEY_A = 'A',
	FR_KEY_B = 'B',
	FR_KEY_C = 'C',
	FR_KEY_D = 'D',
	FR_KEY_E = 'E',
	FR_KEY_F = 'F',
	FR_KEY_G = 'G',
	FR_KEY_H = 'H',
	FR_KEY_I = 'I',
	FR_KEY_J = 'J',
	FR_KEY_K = 'K',
	FR_KEY_L = 'L',
	FR_KEY_M = 'M',
	FR_KEY_N = 'N',
	FR_KEY_O = 'O',
	FR_KEY_P = 'P',
	FR_KEY_Q = 'Q',
	FR_KEY_R = 'R',
	FR_KEY_S = 'S',
	FR_KEY_T = 'T',
	FR_KEY_U = 'U',
	FR_KEY_V = 'V',
	FR_KEY_W = 'W',
	FR_KEY_X = 'X',
	FR_KEY_Y = 'Y',
	FR_KEY_Z = 'Z',

	FR_KEY_LEFT = VK_LEFT,
	FR_KEY_RIGHT = VK_RIGHT,
	FR_KEY_UP = VK_UP,
	FR_KEY_DOWN = VK_DOWN,

	FR_KEY_SPACE = VK_SPACE,

	FR_KEY_LEFT_CONTROL = VK_LCONTROL,
	FR_KEY_RIGHT_CONTROL = VK_RCONTROL,
	FR_KEY_LEFT_SHIFT = VK_LSHIFT,
	FR_KEY_RIGHT_SHIFT = VK_RSHIFT,

	FR_KEY_ESCAPE = VK_ESCAPE
	#else
	FR_KEY_LEFT_MOUSE,
	FR_KEY_RIGHT_MOUSE,

	FR_KEY_A,
	FR_KEY_B,
	FR_KEY_C,
	FR_KEY_D,
	FR_KEY_E,
	FR_KEY_F,
	FR_KEY_G,
	FR_KEY_H,
	FR_KEY_I,
	FR_KEY_J,
	FR_KEY_K,
	FR_KEY_L,
	FR_KEY_M,
	FR_KEY_N,
	FR_KEY_O,
	FR_KEY_P,
	FR_KEY_Q,
	FR_KEY_R,
	FR_KEY_S,
	FR_KEY_T,
	FR_KEY_U,
	FR_KEY_V,
	FR_KEY_W,
	FR_KEY_X,
	FR_KEY_Y,
	FR_KEY_Z,

	FR_KEY_LEFT,
	FR_KEY_RIGHT,
	FR_KEY_UP,
	FR_KEY_DOWN,

	FR_KEY_SPACE,

	FR_KEY_LEFT_CONTROL,
	FR_KEY_RIGHT_CONTROL,
	FR_KEY_LEFT_SHIFT,
	FR_KEY_RIGHT_SHIFT,

	FR_KEY_ESCAPE
	#endif
} FrKey;

typedef enum FrKeyState
{
	FR_KEY_STATE_DOWN,
	FR_KEY_STATE_UP,
} FrKeyState;

// Handlers
typedef void (*FrMouseMoveHandler)(int32_t dx, int32_t dy, void* pUserData);
typedef void (*FrKeyHandler)(FrKey key, FrKeyState state, void* pUserData);
typedef void (*FrResizeHandler)(uint16_t newWidth, uint16_t newHeight, void* pUserData);

typedef struct FrEventHandlers
{
	FrMouseMoveHandler mouseMoveHandler;
	void* pMouseMoveHandlerUserData;

	FrKeyHandler keyHandler;
	void* pKeyHandlerUserData;

	FrResizeHandler resizeHandler;
	void* pResizeHandlerUserData;
} FrEventHandlers;

#ifdef _WIN32
extern HWND windowHandle;
extern HINSTANCE windowInstance;
#else
extern Display* display;
extern Window window;
#endif
extern bool windowResized;
extern bool windowCaptured;
extern FrEventHandlers eventHandlers;

/*
 * Create the window.
 *
 * Parameters:
 * - title: the title of the window.
 * 
 * Returns:
 * - FR_SUCCESS: the window was created successfully
 * - FR_ERROR: an error occurred
 */
FrResult frCreateWindow(const char* title);

/*
 * Destroy the window.
 */
void frDestroyWindow(void);

void frCloseWindow(void);

/*
 * Maximize the window.
 */
void frMaximizeWindow(void);

/*
 * Capture the mouse.
 * 
 * Parameters:
 * - capture: true to capture the mouse, false to release it.
 */
void frCaptureMouse(bool capture);

/*
 * Get the state of a key
 * - key: the key
 */
FrKeyState frGetKeyState(FrKey key);

/* Handlers */

/*
 * Set the mouse move handler.
 * - handler: The handler.
 * - pUserData: The user data.
 */
void frSetMouseMoveHandler(FrMouseMoveHandler handler, void* pUserData);

/*
 * Set the key handler.
 * - handler: The handler.
 * - pUserData: The user data.
 */
void frSetKeyHandler(FrKeyHandler handler, void* pUserData);

/*
 * Set the resize handler.
 * - handler: The handler.
 * - pUserData: The user data.
 */
void frSetResizeHandler(FrResizeHandler handler, void* pUserData);

#endif
