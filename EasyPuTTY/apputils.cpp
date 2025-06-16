#pragma once

#include "apputils.h"
#include "enum.h"

static HINSTANCE g_appInstance;
static TabWindowsInfo* g_tabWindowsInfo = NULL;

// 安全截断宽字符字符串并添加省略号
void TruncateString(const wchar_t* src, wchar_t* dest, size_t maxLength) {
	size_t srcLen = wcslen(src);

	// 如果源字符串长度小于等于最大长度，直接复制
	if (srcLen <= maxLength) {
		wcscpy_s(dest, srcLen + 1, src);
		return;
	}

	// 计算截断位置（留出省略号的空间）
	size_t truncatePos = maxLength - 3;  // 减去 3 为省略号留出空间

	// 检查截断位置是否在代理对中间
	if (truncatePos > 0 &&
		src[truncatePos - 1] >= 0xD800 && src[truncatePos - 1] <= 0xDBFF &&  // 高代理项
		src[truncatePos] >= 0xDC00 && src[truncatePos] <= 0xDFFF) {           // 低代理项
		truncatePos--;  // 调整截断位置以避免破坏代理对
	}

	// 复制截断的字符串
	wcsncpy_s(dest, maxLength + 1, src, truncatePos);

	// 添加省略号
	wcscat_s(dest, maxLength + 1, L"...");
}



int startApp(const wchar_t* appPath, BOOL show) {
	// 进程启动信息
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	if (!show) {
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE; // 隐藏窗口
	}

	// 进程信息
	PROCESS_INFORMATION pi = { 0 };

	// 创建新进程
	if (CreateProcess(
		appPath,            // 程序路径
		NULL,               // 命令行参数
		NULL,               // 进程安全属性
		NULL,               // 线程安全属性
		FALSE,              // 不继承句柄
		CREATE_NO_WINDOW,   // 关键标志：创建无窗口的进程
		NULL,               // 环境变量
		NULL,               // 当前目录
		&si,                // 启动信息
		&pi                 // 进程信息
	)) {

		// 关闭进程和线程句柄
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		if (!show) {
			MessageBoxW(NULL, L"启动成功", L"提示", MB_OK);
		}
		return 0;
	}
	else {
		MessageBoxW(NULL, L"启动失败", L"提示", MB_OK);
		return 1;
	}
}

INT_PTR showDialogBox(HINSTANCE appInstance, TabWindowsInfo* tabWindowsInfo, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc)
{
	g_appInstance = appInstance;
	g_tabWindowsInfo = tabWindowsInfo;
	return DialogBox(appInstance, lpTemplateName, hWndParent, lpDialogFunc);
}

// “枚举”框的消息处理程序。
INT_PTR CALLBACK ENUM(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		createEnum(g_appInstance, g_tabWindowsInfo, hDlg);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			HWND hListView = GetDlgItem(hDlg, ID_ENUM_VIEW);
			int selectedItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);

			// 检索HWND
			LVITEM lvItem = { 0 };
			lvItem.mask = LVIF_PARAM;
			lvItem.iItem = selectedItem;
			ListView_GetItem(hListView, &lvItem);
			SendMessage(g_tabWindowsInfo->parentWinHandle, WM_COMMAND, ID_ENUM_ATTACH, (LPARAM)lvItem.lParam);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_REFRESH) {
			HWND hListView = GetDlgItem(hDlg, ID_ENUM_VIEW);
			DestroyWindow(hListView);
			createEnum(g_appInstance, g_tabWindowsInfo, hDlg);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Session(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_CREDENTIAL_ADD) {
			DialogBox(g_appInstance, MAKEINTRESOURCE(IDD_CREDENTIAL), hDlg, Credential);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Credential(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
