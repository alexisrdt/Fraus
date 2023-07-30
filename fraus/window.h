#ifndef FRAUS_WINDOW_H
#define FRAUS_WINDOW_H

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

#include "utils.h"

#ifdef _WIN32

#include <windows.h>

#endif

// Keyboard key enum
typedef enum FrKey
{
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
	FR_KEY_UNKNOWN
} FrKey;

typedef enum FrKeyStatus
{
	FR_KEY_STATUS_DOWN,
	FR_KEY_STATUS_UP,
} FrKeyStatus;

// Mouse button enum
typedef enum FrMouseButton
{
	FR_MOUSE_LEFT,
	FR_MOUSE_RIGHT
} FrMouseButton;

// Handlers
typedef void(*FrMouseMoveHandler)(uint16_t x, uint16_t y, void* pUserData);
typedef void(*FrClickHandler)(FrMouseButton button, void* pUserData);
typedef void(*FrKeyHandler)(FrKey key, FrKeyStatus status, void* pUserData);
typedef void(*FrResizeHandler)(uint16_t newWidth, uint16_t newHeight, void* pUserData);

typedef struct FrEventHandlers
{
	FrMouseMoveHandler mouseMoveHandler;
	void* pMouseMoveHandlerUserData;

	FrClickHandler clickHandler;
	void* pClickHandlerUserData;

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
#endif
	FrEventHandlers handlers;
	bool resized;

} FrWindow;

/*
 * Create a window
 * - pTitle: the title of the window
 * - pWindow: pointer to a handle for the window
 */
FrResult frCreateWindow(const wchar_t* pTitle, FrWindow* pWindow);

/* Handlers */

/*
 * Set the mouse move handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetMouseMoveHandler(FrWindow* pWindow, FrMouseMoveHandler handler, void* pUserData);

/*
 * Set the click handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetClickHandler(FrWindow* pWindow, FrClickHandler handler, void* pUserData);

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
