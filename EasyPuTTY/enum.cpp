#pragma once

#include "enum.h"
#include <vector>
#include <psapi.h>
#include <tlhelp32.h>
#include <vector>

// 链接所需库
#pragma comment(lib, "psapi.lib")


// 存储窗口信息的结构体
struct WindowInfo {
	HWND hWnd;
	wchar_t title[MAX_PATH];
	DWORD processId;
	wchar_t processName[MAX_PATH];
	wchar_t processPath[MAX_PATH];
};

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

	// 获取窗口标题
	wchar_t title[MAX_PATH] = { 0 };
	if (GetWindowTextW(hWnd, title, sizeof(title) / sizeof(WCHAR)) == 0)
		return TRUE;

	// 获取进程ID
	DWORD processId;
	GetWindowThreadProcessId(hWnd, &processId);

	// 获取存储窗口信息的向量
	std::vector<WindowInfo>* pWindows = reinterpret_cast<std::vector<WindowInfo>*>(lParam);

	// 存储窗口信息
	WindowInfo info = { 0 };
	info.hWnd = hWnd;
	wcscpy_s(info.title, title);
	info.processId = processId;

	// 获取进程名称和路径
	GetProcessInfo(processId, info.processName, info.processPath, MAX_PATH);

	pWindows->push_back(info);

	return TRUE; // 继续枚举
}

// 创建窗口
void createEnum(HINSTANCE hInstance, struct TabWindowsInfo *tabWindowsInfo, HWND parentWindow) {
	RECT rc;
	std::vector<WindowInfo> windows;

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

	 
	// 枚举所有窗口
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));

	// 将窗口信息添加到ListView
	wchar_t pidText[20];

	for (size_t i = 0; i < windows.size(); i++) {
		swprintf_s(pidText, L"%lu", windows[i].processId);
		AddEnumItem(hListView, i, pidText, 0, windows[i].hWnd);

		AddEnumItem(hListView, i, windows[i].title, 1, windows[i].hWnd);
		AddEnumItem(hListView, i, windows[i].processName, 2, windows[i].hWnd);
		AddEnumItem(hListView, i, windows[i].processPath, 3, windows[i].hWnd);
	}


	// 子类化宿主窗口
	//WNDPROC originalProc = (WNDPROC)SetWindowLongPtrW(hostWindow, GWLP_WNDPROC, (LONG_PTR)HostWindowProc);
	// 存储原始窗口过程，用于后续调用
	//SetWindowLongPtrW(hostWindow, GWLP_USERDATA, (LONG_PTR)originalProc);
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