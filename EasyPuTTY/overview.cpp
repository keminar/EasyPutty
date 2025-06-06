#pragma once

#include "overview.h"
#include "common.h"

#define IniName L".\\MultiTab.ini"

// 宿主窗口中列表视图的句柄
HWND hWndListView;


// 宿主窗口的子类化过程
LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// 获取原始窗口过程
	WNDPROC originalProc = (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	switch (uMsg) {
	case WM_GETMAINWINDOW:
		// 向上级窗口查询主窗口句柄
		return (LRESULT)GetParent(hwnd);
	case WM_SIZE:
		// 调整列表视图大小以适应宿主窗口
		if (hWndListView) {
			RECT rc;
			GetClientRect(hwnd, &rc);
			MoveWindow(hWndListView,
				rc.left + 20,          // 左偏移
				rc.top + 60,         // 上偏移
				rc.right - 40,       // 宽度
				rc.bottom - 80,      // 高度
				TRUE);
		}
		break;
	case WM_NOTIFY:
		// 处理列表视图通知消息
		switch (((LPNMHDR)lParam)->code) {
		case LVN_ITEMCHANGED: {
			LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
			if (pnmlv->uNewState & LVIS_SELECTED) {
				// 项目被选中
				wchar_t szText[256] = { 0 };
				LVITEMW lvi = { 0 };
				lvi.mask = LVIF_TEXT;
				lvi.iItem = pnmlv->iItem;
				lvi.iSubItem = 0;
				lvi.pszText = szText;
				lvi.cchTextMax = 256;
				//第一列内容
				SendMessageW(hWndListView, LVM_GETITEMW, 0, (LPARAM)&lvi);
				// 通过发送自定义消息获取主窗口句柄
				HWND mainWindow = (HWND)SendMessage(hwnd, WM_GETMAINWINDOW, 0, 0);
				if (mainWindow) {
					// 转发按钮点击消息到主窗口
					SendMessage(mainWindow, WM_COMMAND, 7002, (LPARAM)&szText[0]);
				}
			}
			return 0;
		}
		}
		break;
	}

	// 其他消息交给原始窗口过程处理
	return CallWindowProcW(originalProc, hwnd, uMsg, wParam, lParam);
}


// 创建窗口
HWND createOverviewWindow(HINSTANCE hInstance, struct TabWindowsInfo *tabWindowsInfo, HWND hostWindow) {
	SendMessageW(hostWindow, WM_SETFONT, (WPARAM)(tabWindowsInfo->editorFontHandle), 0);

	hWndListView = CreateWindow(
		WC_LISTVIEW,
		L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS | LVS_SHOWSELALWAYS,
		50, 100, 550, 300, // 初始位置和大小
		hostWindow,
		(HMENU)7001, // 控件ID
		hInstance,
		NULL
	);

	if (!hWndListView) {
		MessageBoxW(NULL, L"无法创建列表视图", L"错误", MB_OK | MB_ICONERROR);
		return NULL;
	}
	// 设置列表视图扩展样式
	ListView_SetExtendedListViewStyle(hWndListView,
		LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 初始化列表视图列
	InitializeListViewColumns(hWndListView);

	WCHAR app1[256] = { 0 };
	WCHAR app2[256] = { 0 };
	//TCHAR app1[MAX_PATH] = { 0 };
	GetPrivateProfileStringW(L"Program", L"app1", L"", app1, MAX_PATH, IniName);
	GetPrivateProfileStringW(L"Program", L"app2", L"", app2, MAX_PATH, IniName);

	int i = 0;
	// 添加示例数据
	if (wcscmp(app1, L"") != 0) {
		AddListViewItem(hWndListView, i, app1, 0);
		AddListViewItem(hWndListView, i, L"putty", 1);
		AddListViewItem(hWndListView, i, L"putty", 2);
		AddListViewItem(hWndListView, i, L"1", 3);
		i++;
	}

	if (wcscmp(app2, L"") != 0) {
		AddListViewItem(hWndListView, i, app2, 0);
		AddListViewItem(hWndListView, i, L"putty", 1);
		AddListViewItem(hWndListView, i, L"putty", 2);
		AddListViewItem(hWndListView, i, L"1", 3);
		i++;
	}


	AddListViewItem(hWndListView, i, L"explorer .\\", 0);
	AddListViewItem(hWndListView, i, L"资源管理器", 1);
	AddListViewItem(hWndListView, i, L"其他", 2);
	AddListViewItem(hWndListView, i, L"1", 3);
	i++;

	AddListViewItem(hWndListView, i, L"..\\..\\ProxyUI", 0);
	AddListViewItem(hWndListView, i, L"ProxyUI", 1);
	AddListViewItem(hWndListView, i, L"其他", 2);
	AddListViewItem(hWndListView, i, L"1", 3);
	i++;

	AddListViewItem(hWndListView, i, L"C:\\Program Files\\FileZilla FTP Client\\filezilla.exe", 0);
	AddListViewItem(hWndListView, i, L"filezilla", 1);
	AddListViewItem(hWndListView, i, L"其他", 2);
	AddListViewItem(hWndListView, i, L"1", 3);
	i++;

	AddListViewItem(hWndListView, i, L"C:\\HA_EmEditor9x64\\emed106c64\\EmEditor10x64\\EmEditor.exe", 0);
	AddListViewItem(hWndListView, i, L"EmEditor", 1);
	AddListViewItem(hWndListView, i, L"其他", 2);
	AddListViewItem(hWndListView, i, L"1", 3);
	i++;

	AddListViewItem(hWndListView, i, L"cmd", 0);
	AddListViewItem(hWndListView, i, L"命令行", 1);
	AddListViewItem(hWndListView, i, L"其他", 2);
	AddListViewItem(hWndListView, i, L"1", 3);

	// 子类化宿主窗口
	WNDPROC originalProc = (WNDPROC)SetWindowLongPtrW(hostWindow, GWLP_WNDPROC, (LONG_PTR)HostWindowProc);
	// 存储原始窗口过程，用于后续调用
	SetWindowLongPtrW(hostWindow, GWLP_USERDATA, (LONG_PTR)originalProc);

	return hostWindow;
}

// 初始化列表视图列（显式使用宽字符版本）
void InitializeListViewColumns(HWND hWndListView) {
	LVCOLUMNW lvc = { 0 }; // 使用宽字符版本的结构体
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; // 移除错误的 LVIF_TEXT

	// 第1列：
	lvc.iSubItem = 0;
	lvc.cx = 500;
	lvc.pszText = (LPWSTR)L"命令";
	SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);


	// 第2列：
	lvc.iSubItem = 1;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"简称";
	SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);

	// 第3列：
	lvc.iSubItem = 2;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"分类";
	SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 2, (LPARAM)&lvc);

	// 第4列：
	lvc.iSubItem = 3;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"发送ctl+space";
	SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 3, (LPARAM)&lvc);

	//支持sftp，winscp等
}

// 添加列表项（显式使用宽字符版本）
void AddListViewItem(HWND hWndListView, int nItem, const wchar_t* pszText, int nSubItem) {
	LVITEMW lvi = { 0 };
	lvi.mask = LVIF_TEXT;
	lvi.iItem = nItem;
	lvi.iSubItem = nSubItem;
	lvi.pszText = (LPWSTR)pszText;
	if (nSubItem == 0) {
		SendMessageW(hWndListView, LVM_INSERTITEMW, 0, (LPARAM)&lvi);
	}
	else {
		// 设置子项
		SendMessageW(hWndListView, LVM_SETITEMW, 0, (LPARAM)&lvi);

	}
}