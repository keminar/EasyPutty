#pragma once

#include "overview.h"

#define IniName L".\\EasyPuTTY.ini"


// �������ڵ����໯����
LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// ��ȡԭʼ���ڹ���
	WNDPROC originalProc = (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	switch (uMsg) {
	case WM_GETMAINWINDOW:
		// ���ϼ����ڲ�ѯ�����ھ��
		return (LRESULT)GetParent(hwnd);
	case WM_SIZE: {
		HWND hListView = GetDlgItem(hwnd, ID_LIST_VIEW);
		// �����б���ͼ��С����Ӧ��������
		if (hListView) {
			RECT rc;
			GetClientRect(hwnd, &rc);
			MoveWindow(hListView, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
		}
		break;
	}
	case WM_NOTIFY: {
		// ���� ListView ֪ͨ��Ϣ
		LPNMHDR pnmh = (LPNMHDR)lParam;
		HWND hListView = GetDlgItem(hwnd, ID_LIST_VIEW);
		if (pnmh->hwndFrom == hListView) {
			if  (pnmh->code == NM_DBLCLK) {
				// ˫���¼�����
				LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)lParam;
				if (pnmia->iItem != -1) {
					execCommand(hwnd, hListView, pnmia->iItem);
				}
			}
			else if (pnmh->code == LVN_KEYDOWN) {
				LPNMLVKEYDOWN pnmlvkd = (LPNMLVKEYDOWN)lParam;
				if (pnmlvkd->wVKey == VK_RETURN) {  // �س���
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

	// ������Ϣ����ԭʼ���ڹ��̴���
	return CallWindowProcW(originalProc, hwnd, uMsg, wParam, lParam);
}

// ִ���������±�ǩ��
void execCommand(HWND hwnd, HWND hListView, int selectedItem) {
	wchar_t szText[256] = { 0 };
	ListView_GetItemText(hListView, selectedItem, 0, szText, sizeof(szText));
	// ͨ�������Զ�����Ϣ��ȡ�����ھ��
	HWND mainWindow = (HWND)SendMessage(hwnd, WM_GETMAINWINDOW, 0, 0);
	if (mainWindow) {
		// ת����ť�����Ϣ��������
		SendMessage(mainWindow, WM_COMMAND, ID_LIST_ATTACH, (LPARAM)&szText[0]);
	}
}

// ��������
void InitOverview(HINSTANCE hInstance, struct TabWindowsInfo *tabWindowsInfo, HWND hostWindow) {
	SendMessageW(hostWindow, WM_SETFONT, (WPARAM)(tabWindowsInfo->editorFontHandle), 0);

	HWND hListView = CreateWindowW(
		WC_LISTVIEW,
		L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS | LVS_SHOWSELALWAYS,
		0, 0, 500, 300, // ��ʼλ�úʹ�С
		hostWindow,
		(HMENU)ID_LIST_VIEW, // �ؼ�ID
		hInstance,
		NULL
	);

	if (!hListView) {
		MessageBoxW(NULL, L"�޷������б���ͼ", L"����", MB_OK | MB_ICONERROR);
		return;
	}
	// �����б���ͼ��չ��ʽ
	ListView_SetExtendedListViewStyle(hListView,
		LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// ��ʼ���б���ͼ��
	InitializeListViewColumns(hListView);

	WCHAR app1[256] = { 0 };
	WCHAR app2[256] = { 0 };
	//TCHAR app1[MAX_PATH] = { 0 };
	GetPrivateProfileStringW(L"Program", L"app1", L"", app1, MAX_PATH, IniName);
	GetPrivateProfileStringW(L"Program", L"app2", L"", app2, MAX_PATH, IniName);

	int i = 0;
	// ���ʾ������
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
	AddListViewItem(hListView, i, L"��Դ������", 1);
	AddListViewItem(hListView, i, L"����", 2);
	AddListViewItem(hListView, i, L"1", 3);
	i++;

	AddListViewItem(hListView, i, L"..\\..\\ProxyUI", 0);
	AddListViewItem(hListView, i, L"ProxyUI", 1);
	AddListViewItem(hListView, i, L"����", 2);
	AddListViewItem(hListView, i, L"1", 3);
	i++;

	AddListViewItem(hListView, i, L"C:\\Program Files\\FileZilla FTP Client\\filezilla.exe", 0);
	AddListViewItem(hListView, i, L"filezilla", 1);
	AddListViewItem(hListView, i, L"����", 2);
	AddListViewItem(hListView, i, L"1", 3);
	i++;

	AddListViewItem(hListView, i, L"C:\\HA_EmEditor9x64\\emed106c64\\EmEditor10x64\\EmEditor.exe", 0);
	AddListViewItem(hListView, i, L"EmEditor", 1);
	AddListViewItem(hListView, i, L"����", 2);
	AddListViewItem(hListView, i, L"1", 3);
	i++;

	AddListViewItem(hListView, i, L"cmd", 0);
	AddListViewItem(hListView, i, L"������", 1);
	AddListViewItem(hListView, i, L"����", 2);
	AddListViewItem(hListView, i, L"1", 3);

	// ���໯��������
	WNDPROC originalProc = (WNDPROC)SetWindowLongPtrW(hostWindow, GWLP_WNDPROC, (LONG_PTR)HostWindowProc);
	// �洢ԭʼ���ڹ��̣����ں�������
	SetWindowLongPtrW(hostWindow, GWLP_USERDATA, (LONG_PTR)originalProc);
}

// ��ʼ���б���ͼ��
void InitializeListViewColumns(HWND hWndListView) {
	LVCOLUMNW lvc = { 0 }; // ʹ�ÿ��ַ��汾�Ľṹ��
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; // �Ƴ������ LVIF_TEXT

	// ��1�У�
	lvc.iSubItem = 0;
	lvc.cx = 500;
	lvc.pszText = (LPWSTR)L"����";
	ListView_InsertColumn(hWndListView, 0, &lvc);
	//SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);


	// ��2�У�
	lvc.iSubItem = 1;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"���";
	ListView_InsertColumn(hWndListView, 1, &lvc);
	//SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);

	// ��3�У�
	lvc.iSubItem = 2;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"����";
	ListView_InsertColumn(hWndListView, 2, &lvc);
	//SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 2, (LPARAM)&lvc);

	// ��4�У�
	lvc.iSubItem = 3;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"����ctl+space";
	ListView_InsertColumn(hWndListView, 3, &lvc);
	//SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 3, (LPARAM)&lvc);

	//֧��sftp��winscp��
}

// ����б���
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