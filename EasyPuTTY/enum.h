#pragma once

#include "framework.h"
#include "apputils.h"
#include "lang_manager.h"

typedef struct {
	HWND hWnd;
	wchar_t title[MAX_PATH];
	DWORD processId;
	wchar_t processName[MAX_PATH];
	wchar_t processPath[MAX_PATH];
} WindowInfo;

typedef struct {
	WindowInfo* data;
	size_t size;
	size_t capacity;
} WindowVector;

void createEnum(HINSTANCE hInstance, struct TabWindowsInfo *tabWindowsInfo, HWND hostWindow);
void InitEnumColumns(HWND hWndListView);
void AddEnumItem(HWND hWndListView, int nItem, const wchar_t* pszText, int nSubItem, HWND hWnd);

WindowVector* InitWindowVector(size_t capactiy);
void FreeWindowVector(WindowVector* vec);
int AddWindowVector(WindowVector* vec, const WindowInfo* info);
size_t WindowVectorSize(WindowVector* vec);
WindowInfo* WindowVectorGet(WindowVector* vec, size_t index);