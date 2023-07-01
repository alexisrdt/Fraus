#ifndef FRAUS_WINDOW_H
#define FRAUS_WINDOW_H

#include <wchar.h>

#include "utils.h"

#ifdef _WIN32

#include <windows.h>

typedef HWND FrWindow;

#endif

/*
 * Create a window
 * - pTitle: the title of the window
 * - pWindow: pointer to a handle for the window
 */
FrResult frCreateWindow(const wchar_t* pTitle, FrWindow* pWindow);

/*
 * Main loop of the program
 * - pReturnValue: pointer to a handle for the return value (can be NULL)
 */
void frMainLoop(int* pReturnValue);

#endif
