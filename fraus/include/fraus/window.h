#ifndef FRAUS_WINDOW_H
#define FRAUS_WINDOW_H

#include <stdint.h>

#include "utils.h"

#include <windows.h>

// Keyboard key enum
typedef enum FrKey
{
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
} FrKey;

typedef enum FrKeyState
{
	FR_KEY_STATE_DOWN,
	FR_KEY_STATE_UP,
} FrKeyState;

typedef struct FrWindow
{
	HWND window;

	bool resized;
	bool captured;
} FrWindow;

extern HINSTANCE windowInstance;

typedef struct FrApplication FrApplication;

FrResult frInitializeWindow(void);

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
FrResult frCreateWindow(FrApplication* application, FrWindow* window, const char* title);

/*
 * Destroy the window.
 */
void frDestroyWindow(FrWindow* window);

void frCloseWindow(FrWindow* window);

/*
 * Maximize the window.
 */
void frMaximizeWindow(FrWindow* window);

/*
 * Capture the mouse.
 * 
 * Parameters:
 * - capture: true to capture the mouse, false to release it.
 */
void frCaptureMouse(FrWindow* window, bool capture);

/*
 * Get the state of a key
 * - key: the key
 */
FrKeyState frGetKeyState(FrKey key);

#endif
