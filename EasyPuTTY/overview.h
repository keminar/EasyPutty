#pragma once

#include "framework.h"
#include "common.h"


LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitOverview(HINSTANCE hInstance, struct TabWindowsInfo *tabWindowsInfo, HWND hostWindow);
void InitializeListViewColumns(HWND hWndListView);
void AddListViewItem(HWND hWndListView, int nItem, const wchar_t* pszText, int nSubItem);
void execCommand(HWND hwnd, HWND hListView, int selectedItem);