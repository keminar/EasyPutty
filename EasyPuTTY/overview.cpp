#pragma once

#include "overview.h"

#define IniName L".\\EasyPuTTY.ini"


// 宿主窗口的子类化过程
LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// 获取原始窗口过程
	WNDPROC originalProc = (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	switch (uMsg) {
	case WM_GETMAINWINDOW:
		// 向上级窗口查询主窗口句柄
		return (LRESULT)GetParent(hwnd);
	case WM_SIZE: {
		HWND hListView = GetDlgItem(hwnd, ID_LIST_VIEW);
		// 调整列表视图大小以适应宿主窗口
		if (hListView) {
			RECT rc;
			GetClientRect(hwnd, &rc);
			MoveWindow(hListView, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
		}
		break;
	}
	case WM_NOTIFY: {
		// 处理 ListView 通知消息
		LPNMHDR pnmh = (LPNMHDR)lParam;
		HWND hListView = GetDlgItem(hwnd, ID_LIST_VIEW);
		if (pnmh->hwndFrom == hListView) {
			if  (pnmh->code == NM_DBLCLK) {
				// 双击事件处理
				LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)lParam;
				if (pnmia->iItem != -1) {
					execCommand(hwnd, hListView, pnmia->iItem);
				}
			}
			else if (pnmh->code == LVN_KEYDOWN) {
				LPNMLVKEYDOWN pnmlvkd = (LPNMLVKEYDOWN)lParam;
				if (pnmlvkd->wVKey == VK_RETURN) {  // 回车键
					int selectedItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
					if (selectedItem != -1) {
						execCommand(hwnd, hListView, selectedItem);
					}
				}
			}
		}
		}
		break;
	}

	// 其他消息交给原始窗口过程处理
	return CallWindowProcW(originalProc, hwnd, uMsg, wParam, lParam);
}

// 执行命令在新标签打开
void execCommand(HWND hwnd, HWND hListView, int selectedItem) {
	wchar_t szText[256] = { 0 };
	ListView_GetItemText(hListView, selectedItem, 0, szText, sizeof(szText));
	// 通过发送自定义消息获取主窗口句柄
	HWND mainWindow = (HWND)SendMessage(hwnd, WM_GETMAINWINDOW, 0, 0);
	if (mainWindow) {
		// 转发按钮点击消息到主窗口
		SendMessage(mainWindow, WM_COMMAND, ID_LIST_ATTACH, (LPARAM)&szText[0]);
	}
}

// 创建窗口
void InitOverview(HINSTANCE hInstance, struct TabWindowsInfo *tabWindowsInfo, HWND hostWindow) {
	SendMessageW(hostWindow, WM_SETFONT, (WPARAM)(tabWindowsInfo->editorFontHandle), 0);

	HWND hListView = CreateWindowW(
		WC_LISTVIEW,
		L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS | LVS_SHOWSELALWAYS,
		0, 0, 500, 300, // 初始位置和大小
		hostWindow,
		(HMENU)ID_LIST_VIEW, // 控件ID
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
	InitializeListViewColumns(hListView);

	WCHAR app1[256] = { 0 };
	WCHAR app2[256] = { 0 };
	//TCHAR app1[MAX_PATH] = { 0 };
	GetPrivateProfileStringW(L"Program", L"app1", L"", app1, MAX_PATH, IniName);
	GetPrivateProfileStringW(L"Program", L"app2", L"", app2, MAX_PATH, IniName);

	int i = 0;
	// 添加示例数据
	if (wcscmp(app1, L"") != 0) {
		AddListViewItem(hListView, i, app1, 0);
		AddListViewItem(hListView, i, L"putty", 1);
		AddListViewItem(hListView, i, L"putty", 2);
		AddListViewItem(hListView, i, L"1", 3);
		i++;
	}

	if (wcscmp(app2, L"") != 0) {
		AddListViewItem(hListView, i, app2, 0);
		AddListViewItem(hListView, i, L"putty", 1);
		AddListViewItem(hListView, i, L"putty", 2);
		AddListViewItem(hListView, i, L"1", 3);
		i++;
	}


	AddListViewItem(hListView, i, L"explorer .\\", 0);
	AddListViewItem(hListView, i, L"资源管理器", 1);
	AddListViewItem(hListView, i, L"其他", 2);
	AddListViewItem(hListView, i, L"1", 3);
	i++;

	AddListViewItem(hListView, i, L"..\\..\\ProxyUI", 0);
	AddListViewItem(hListView, i, L"ProxyUI", 1);
	AddListViewItem(hListView, i, L"其他", 2);
	AddListViewItem(hListView, i, L"1", 3);
	i++;

	AddListViewItem(hListView, i, L"C:\\Program Files\\FileZilla FTP Client\\filezilla.exe", 0);
	AddListViewItem(hListView, i, L"filezilla", 1);
	AddListViewItem(hListView, i, L"其他", 2);
	AddListViewItem(hListView, i, L"1", 3);
	i++;

	AddListViewItem(hListView, i, L"C:\\HA_EmEditor9x64\\emed106c64\\EmEditor10x64\\EmEditor.exe", 0);
	AddListViewItem(hListView, i, L"EmEditor", 1);
	AddListViewItem(hListView, i, L"其他", 2);
	AddListViewItem(hListView, i, L"1", 3);
	i++;

	AddListViewItem(hListView, i, L"cmd", 0);
	AddListViewItem(hListView, i, L"命令行", 1);
	AddListViewItem(hListView, i, L"其他", 2);
	AddListViewItem(hListView, i, L"1", 3);

	// 子类化宿主窗口
	WNDPROC originalProc = (WNDPROC)SetWindowLongPtrW(hostWindow, GWLP_WNDPROC, (LONG_PTR)HostWindowProc);
	// 存储原始窗口过程，用于后续调用
	SetWindowLongPtrW(hostWindow, GWLP_USERDATA, (LONG_PTR)originalProc);
}

// 初始化列表视图列
void InitializeListViewColumns(HWND hWndListView) {
	LVCOLUMNW lvc = { 0 }; // 使用宽字符版本的结构体
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; // 移除错误的 LVIF_TEXT

	// 第1列：
	lvc.iSubItem = 0;
	lvc.cx = 500;
	lvc.pszText = (LPWSTR)L"命令";
	ListView_InsertColumn(hWndListView, 0, &lvc);
	//SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);


	// 第2列：
	lvc.iSubItem = 1;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"简称";
	ListView_InsertColumn(hWndListView, 1, &lvc);
	//SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);

	// 第3列：
	lvc.iSubItem = 2;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"分类";
	ListView_InsertColumn(hWndListView, 2, &lvc);
	//SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 2, (LPARAM)&lvc);

	// 第4列：
	lvc.iSubItem = 3;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"发送ctl+space";
	ListView_InsertColumn(hWndListView, 3, &lvc);
	//SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 3, (LPARAM)&lvc);

	//支持sftp，winscp等
}

// 添加列表项
void AddListViewItem(HWND hWndListView, int nItem, const wchar_t* pszText, int nSubItem) {
	if (nSubItem == 0) {
		LVITEMW lvi = { 0 };
		lvi.mask = LVIF_TEXT;
		lvi.iItem = nItem;
		lvi.iSubItem = nSubItem;
		lvi.pszText = (LPWSTR)pszText;
		ListView_InsertItem(hWndListView, &lvi);
	}
	else {
		ListView_SetItemText(hWndListView, nItem, nSubItem, (LPWSTR)pszText);
	}
}