#pragma once

#include "enum.h"
#include <psapi.h>
#include <tlhelp32.h>

// 链接所需库
#pragma comment(lib, "psapi.lib")

// 全局变量:
WCHAR myWindowClass[256];            // 主窗口类名
HWND enumWindow;                     // 当前窗口句柄

// 获取进程名称和路径
BOOL GetProcessInfo(DWORD processId, wchar_t* processName, wchar_t* processPath, DWORD bufferSize) {
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	if (hProcess == NULL) {
		wcscpy_s(processName, bufferSize, L"无法访问");
		wcscpy_s(processPath, bufferSize, L"无法访问");
		return FALSE;
	}

	// 获取进程名称
	if (GetModuleFileNameExW(hProcess, NULL, processPath, bufferSize)) {
		// 提取文件名（不含路径）
		wchar_t* fileName = wcsrchr(processPath, L'\\');
		if (fileName) {
			wcscpy_s(processName, bufferSize, fileName + 1);
		}
		else {
			wcscpy_s(processName, bufferSize, processPath);
		}
	}
	else {
		wcscpy_s(processName, bufferSize, L"未知");
		wcscpy_s(processPath, bufferSize, L"未知");
	}

	CloseHandle(hProcess);
	return TRUE;
}

// 窗口枚举回调函数
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
	// 检查窗口是否可见且有标题
	if (!IsWindowVisible(hWnd))
		return TRUE;
	// 排除自身窗口
	if (enumWindow == hWnd)
		return TRUE;

	// 排除自身主程序
	wchar_t className[256] = { 0 };
	GetClassNameW(hWnd, className, ARRAYSIZE(className));
	if (wcsstr(className, myWindowClass) != NULL) {
		return TRUE;
	}

	// 获取窗口标题
	wchar_t title[MAX_PATH] = { 0 };
	if (GetWindowTextW(hWnd, title, sizeof(title) / sizeof(WCHAR)) == 0)
		return TRUE;

	// 获取进程ID
	DWORD processId;
	GetWindowThreadProcessId(hWnd, &processId);

	// 获取存储窗口信息的向量
	WindowVector* pWindows = (WindowVector*)lParam;

	// 存储窗口信息
	WindowInfo info = { 0 };
	info.hWnd = hWnd;
	wcscpy_s(info.title, title);
	info.processId = processId;

	// 获取进程名称和路径
	GetProcessInfo(processId, info.processName, info.processPath, MAX_PATH);

	// 已知需要排掉的进程
	if (wcsstr(info.processName, L"explorer.exe") != NULL) {
		if (wcsstr(title, L"Program Manager") != NULL) {//桌面管理
			return TRUE;
		}
	}
	else if (wcsstr(info.processName, L"ApplicationFrameHost.exe") != NULL) {
		return TRUE;
	}
	else if (wcsstr(info.processName, L"ShellExperienceHost.exe") != NULL) {
		return TRUE;
	}
	else if (wcsstr(info.processName, L"WindowsInternal.ComposableShell.Experiences.TextInput.InputApp.exe") != NULL) {
		return TRUE;
	}
	else if (wcsstr(info.processName, L"SystemSettings.exe") != NULL) {
		return TRUE;
	}
	else if (wcsstr(info.processName, L"MicrosoftEdge") != NULL) {
		return TRUE;
	}
	else if (wcsstr(info.processName, L"WeMail.exe") != NULL) {//微信邮件
		return TRUE;
	}
	/*else if (wcsstr(info.processName, L"无法访问") != NULL) {
		return TRUE;
	}
	else if (wcsstr(info.processName, L"未知") != NULL) {//win7一些进程会得到这个
		return TRUE;
	}*/
	
	AddWindowVector(pWindows, &info);

	return TRUE; // 继续枚举
}

// 创建窗口
void createEnum(HINSTANCE hInstance, struct TabWindowsInfo *tabWindowsInfo, HWND parentWindow) {
	RECT rc;
	WindowVector* windows;
	WindowInfo* info;

	enumWindow = parentWindow;
	GetClientRect(parentWindow, &rc);
	HWND hListView = CreateWindowW(
		WC_LISTVIEW,
		L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS | LVS_SHOWSELALWAYS,
		rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top - 50, // 初始位置和大小
		parentWindow,
		(HMENU)ID_ENUM_VIEW, // 控件ID
		hInstance,
		NULL
	);

	if (!hListView) {
		MessageBoxW(NULL, L"无法创建列表视图", L"错误", MB_OK | MB_ICONERROR);
		return;
	}
	// 设置列表视图扩展样式
	ListView_SetExtendedListViewStyle(hListView,
		LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 初始化列表视图列
	InitEnumColumns(hListView);

	// 本程序类名
	LoadStringW(hInstance, IDC_EASYPUTTY, myWindowClass, sizeof(myWindowClass)/sizeof(wchar_t));
	 
	// 枚举所有窗口
	windows = InitWindowVector(10);
	EnumWindows(EnumWindowsProc, (LPARAM)windows);

	// 将窗口信息添加到ListView
	wchar_t pidText[20];

	for (size_t i = 0; i < windows->size; i++) {
		info = WindowVectorGet(windows, i);
		swprintf_s(pidText, L"%lu", info->processId);
		AddEnumItem(hListView, i, pidText, 0, info->hWnd);

		AddEnumItem(hListView, i, info->title, 1, info->hWnd);
		AddEnumItem(hListView, i, info->processName, 2, info->hWnd);
		AddEnumItem(hListView, i, info->processPath, 3, info->hWnd);
	}
	FreeWindowVector(windows);
}

// 初始化列表视图列
void InitEnumColumns(HWND hWndListView) {
	LVCOLUMNW lvc = { 0 };
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; 

	// 第1列：
	lvc.iSubItem = 0;
	lvc.cx = 100;
	lvc.pszText = (LPWSTR)L"PID";
	ListView_InsertColumn(hWndListView, 0, &lvc);


	// 第2列：
	lvc.iSubItem = 1;
	lvc.cx = 225;
	lvc.pszText = (LPWSTR)L"窗口";
	ListView_InsertColumn(hWndListView, 1, &lvc);

	// 第3列：
	lvc.iSubItem = 2;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"进程名";
	ListView_InsertColumn(hWndListView, 2, &lvc);

	// 第4列：
	lvc.iSubItem = 3;
	lvc.cx = 460;
	lvc.pszText = (LPWSTR)L"路径";
	ListView_InsertColumn(hWndListView, 3, &lvc);
}

// 添加列表项
void AddEnumItem(HWND hWndListView, int nItem, const wchar_t* pszText, int nSubItem, HWND hWnd) {
	if (nSubItem == 0) {
		LVITEMW lvi = { 0 };
		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		lvi.iItem = nItem;
		lvi.iSubItem = nSubItem;
		lvi.pszText = (LPWSTR)pszText;
		lvi.lParam = (LPARAM)hWnd;
		ListView_InsertItem(hWndListView, &lvi);
	}
	else {
		ListView_SetItemText(hWndListView, nItem, nSubItem, (LPWSTR)pszText);
	}
}


WindowVector* InitWindowVector(size_t capacity)
{
	WindowVector* vec = (WindowVector*)malloc(sizeof(WindowVector));
	if (!vec) {
		return NULL;
	}
	WindowInfo* data = (WindowInfo*)malloc(capacity * sizeof(WindowInfo));
	if (!data) {
		free(vec);
		return NULL;
	}
	vec->data = data;
	vec->size = 0;
	vec->capacity = capacity;
	return vec;
}

void FreeWindowVector(WindowVector* vec)
{
	if (vec) {
		if (vec->data) free(vec->data);
		free(vec);
	}
}

// static 限制函数本文件可见，不在头文件定义
static int resize(WindowVector* vec, size_t newCapacity)
{
	WindowInfo* newData = (WindowInfo*)realloc(vec->data, newCapacity * sizeof(WindowInfo));
	if (!newData) return 0;

	vec->data = newData;
	vec->capacity = newCapacity;
	return 1;
}

int AddWindowVector(WindowVector* vec, const WindowInfo* info)
{
	if (!vec) return 0;
	if (vec->size >= vec->capacity) {
		size_t newCapacity = vec->capacity ? vec->capacity * 2 : 10;
		if (!resize(vec, newCapacity)) return 0;
	}
	memcpy(&vec->data[vec->size], info, sizeof(WindowInfo));
	vec->size++;
	return 1;
}

size_t WindowVectorSize(WindowVector* vec)
{
	if (!vec) return 0;
	return vec->size;
}

WindowInfo* WindowVectorGet(WindowVector* vec, size_t index)
{
	if (!vec) return NULL;
	if (index >= vec->size) return  NULL;
	return &vec->data[index];
}