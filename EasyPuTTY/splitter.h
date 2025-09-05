#pragma once

#include "framework.h"

HWND createSplitWindow(HINSTANCE hInstance, HWND hWnd);
void registerClass();
void CreateChildWindows(HWND hWnd);
void ArrangeWindows();
LRESULT CALLBACK SplitWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SplitterProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ScrollbarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
int GetSplitterAtPoint(POINT pt);
void insertSplitWindow(HWND hWnd, int pos);

int GetWindowRegion(HWND hWnd);
int CalculateOverlapArea(const RECT* rc1, const RECT* rc2);
HWND GetHandleByRegion(int region);
void SetHandleByRegion(int region, HWND hWnd);
void SwapRegionHandles(int regionA, int regionB);
BOOL IsManagedPuTTY(HWND hWnd);
void CALLBACK MoveSizeChangeHookProc(
	HWINEVENTHOOK hHook, DWORD event,
	HWND hWnd, LONG idObject, LONG idChild,
	DWORD idEventThread, DWORD dwmsEventTime
);
void StartPuTTYHooks();
void StopPuTTYHooks();