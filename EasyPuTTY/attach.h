#pragma once

#include "framework.h"

HWND createAttachWindow(HINSTANCE hInstance, HWND parentWindow, BOOL show);
BOOL CALLBACK EnumPuTTYWindows(HWND hwnd, LPARAM lParam);
HWND FindPuttyWindow();
HWND createPuttyWindow(HINSTANCE hInstance, HWND hostWindow, const wchar_t* puttyPath);