#ifndef FRAUS_WINDOW_H
#define FRAUS_WINDOW_H

#include "utils.h"

#ifdef _WIN32

#include <windows.h>

typedef HWND FrWindow;

#endif

FrResult frCreateWindow(FrWindow* pWindow);
void frMainLoop(int* pReturnValue);

#endif
