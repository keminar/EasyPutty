#pragma once

#include "attach.h"

DWORD  dwThreadId;
wchar_t commandLine[MAX_PATH];

// 回调函数：用于查找PuTTY窗口
BOOL CALLBACK EnumPuTTYWindows(HWND hwnd, LPARAM lParam) {
	WCHAR szTitle[256] = { 0 };
	GetWindowTextW(hwnd, szTitle, sizeof(szTitle) / sizeof(WCHAR));
	// 跳过空标题窗口
	if (wcscmp(szTitle, L"") == 0) {
		return TRUE;
	}
	if (wcsstr(szTitle, L"资源管理器") != NULL) {
		// 资源管理器进程id匹配不到,也不符合IsWindowVisible
		if (wcsstr(commandLine, L"explorer") != NULL) {
			*(HWND*)lParam = hwnd;
			return FALSE; // 停止枚举
		}
	}
	else if (IsWindowVisible(hwnd)) {
		DWORD processId;
		// 获取进程 ID
		GetWindowThreadProcessId(hwnd, &processId);
		if (dwThreadId == processId) {
			*(HWND*)lParam = hwnd;
			return FALSE; // 停止枚举
		}
	}
	return TRUE;
}

// 查找PuTTY窗口句柄
HWND FindPuttyWindow() {
	HWND hwnd = NULL;
	EnumWindows(EnumPuTTYWindows, (LPARAM)&hwnd);
	return hwnd;
}

HWND createPuttyWindow(HINSTANCE hInstance, HWND hostWindow, const wchar_t* puttyPath) {
	// 启动PuTTY进程
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;

	// 创建进程启动信息
	wcscpy_s(commandLine, MAX_PATH, puttyPath);

	if (!CreateProcessW(
		NULL,
		commandLine,
		NULL,
		NULL,
		FALSE,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&si,
		&pi
	)) {
		MessageBoxW(NULL, L"无法启动进程", L"错误", MB_OK | MB_ICONERROR);
		return NULL;
	}

	// 关闭进程和线程句柄（不需要进一步操作）
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	dwThreadId = pi.dwProcessId;

	// 等待PuTTY窗口创建（需要短暂延时）
	HWND puttyHwnd = NULL;
	for (int i = 0; i < 30 && !puttyHwnd; i++) {
		puttyHwnd = FindPuttyWindow();
		Sleep(100);
	}

	// 查找PuTTY窗口
	if (!puttyHwnd) {
		return NULL;
	}
	// 嵌入PuTTY窗口到宿主窗口
	SetParent(puttyHwnd, hostWindow);

	// 调整PuTTY窗口样式
	LONG_PTR style = GetWindowLongPtr(puttyHwnd, GWL_STYLE);
	style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

	// 应用新样式
	SetWindowLongPtr(puttyHwnd, GWL_STYLE, style);
	return puttyHwnd;
}
