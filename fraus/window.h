#ifndef FRAUS_WINDOW_H
#define FRAUS_WINDOW_H

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
	FR_KEY_UNKNOWN
} FrKey;

// Mouse button enum
typedef enum FrMouseButton
{
	FR_MOUSE_LEFT,
	FR_MOUSE_RIGHT
} FrMouseButton;

// Handlers
typedef void(*FrMouseMoveHandler)(uint16_t x, uint16_t y);
typedef void(*FrClickHandler)(FrMouseButton button);
typedef void(*FrKeyHandler)(FrKey key);
typedef void(*FrResizeHandler)(uint16_t newWidth, uint16_t newHeight);

typedef struct FrEventHandlers
{
	FrMouseMoveHandler mouseMoveHandler;
	FrClickHandler clickHandler;
	FrKeyHandler keyHandler;
	FrResizeHandler resizeHandler;
} FrEventHandlers;

// Window type
typedef struct FrWindow
{
#ifdef _WIN32
	HWND handle;
#endif
	FrEventHandlers handlers;

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
void frSetMouseMoveHandler(FrWindow* pWindow, FrMouseMoveHandler handler);

/*
 * Set the click handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetClickHandler(FrWindow* pWindow, FrClickHandler handler);

/*
 * Set the key handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetKeyHandler(FrWindow* pWindow, FrKeyHandler handler);

/*
 * Set the resize handler
 * - pWindow: pointer to the window
 * - handler: the handler
 */
void frSetResizeHandler(FrWindow* pWindow, FrResizeHandler handler);

/*
 * Main loop of the program
 * Returns the exit value of the program
 */
int frMainLoop();

#endif
