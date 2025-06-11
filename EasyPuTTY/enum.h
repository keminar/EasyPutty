#pragma once

#include "framework.h"
#include "common.h"

void createEnum(HINSTANCE hInstance, struct TabWindowsInfo *tabWindowsInfo, HWND hostWindow);
void InitEnumColumns(HWND hWndListView);
void AddEnumItem(HWND hWndListView, int nItem, const wchar_t* pszText, int nSubItem, HWND hWnd);