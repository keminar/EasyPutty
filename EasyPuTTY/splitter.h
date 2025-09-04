#pragma once

#include "framework.h"

HWND createSplitWindow(HINSTANCE hInstance, HWND hWnd);
void registerClass();
void CreateChildWindows(HWND parentWindow);
void ArrangeWindows();
LRESULT CALLBACK SplitWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SplitterProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ScrollbarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
int GetSplitterAtPoint(POINT pt);
void insertSplitWindow(HWND hWnd, int pos);