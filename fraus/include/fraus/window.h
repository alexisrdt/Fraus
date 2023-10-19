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

	FR_KEY_ESCAPE,

	FR_KEY_UNKNOWN
} FrKey;

typedef enum FrKeyState
{
	FR_KEY_STATE_DOWN,
	FR_KEY_STATE_UP,
} FrKeyState;

// Handlers
typedef void(*FrMouseMoveHandler)(int32_t dx, int32_t dy, void* pUserData);
typedef void(*FrKeyHandler)(FrKey key, FrKeyState state, void* pUserData);
typedef void(*FrResizeHandler)(uint16_t newWidth, uint16_t newHeight, void* pUserData);

typedef struct FrEventHandlers
{
	FrMouseMoveHandler mouseMoveHandler;
	void* pMouseMoveHandlerUserData;

	FrKeyHandler keyHandler;
	void* pKeyHandlerUserData;

	FrResizeHandler resizeHandler;
	void* pResizeHandlerUserData;
} FrEventHandlers;

// Window type
typedef struct FrWindow
{
#ifdef _WIN32
	HWND handle;
#else
	Display* pDisplay;
	Window window;
#endif
	FrEventHandlers handlers;
	bool resized;
	bool capture;

} FrWindow;

/*
 * Create a window
 * - pTitle: the title of the window
 * - pWindow: pointer to a handle for the window
 */
FrResult frCreateWindow(const char* pTitle, FrWindow* pWindow);

/*
 * Destroy a window
 * - pWindow: pointer to the window
 */
void frDestroyWindow(FrWindow* pWindow);

/*
 * Capture the mouse
 * - pWindow: pointer to the window
 * - capture: true to capture the mouse, false to release it
 */
void frCaptureMouse(FrWindow* pWindow, bool capture);

/*
 * Get the state of a key
 * - key: the key
 */
FrKeyState frGetKeyState(FrKey key);

/* Handlers */

/*
 * Set the mouse move handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetMouseMoveHandler(FrWindow* pWindow, FrMouseMoveHandler handler, void* pUserData);

/*
 * Set the key handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetKeyHandler(FrWindow* pWindow, FrKeyHandler handler, void* pUserData);

/*
 * Set the resize handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetResizeHandler(FrWindow* pWindow, FrResizeHandler handler, void* pUserData);

#endif
