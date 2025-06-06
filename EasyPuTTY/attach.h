#pragma once

#include "framework.h"

BOOL CALLBACK EnumPuTTYWindows(HWND hwnd, LPARAM lParam);
HWND FindPuttyWindow();
HWND createPuttyWindow(HINSTANCE hInstance, HWND hostWindow, const wchar_t* puttyPath);