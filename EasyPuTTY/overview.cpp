#pragma once

#include "overview.h"
#include "common.h"

#define IniName L".\\MultiTab.ini"

// �����������б���ͼ�ľ��
HWND hWndListView;


// �������ڵ����໯����
LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// ��ȡԭʼ���ڹ���
	WNDPROC originalProc = (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	switch (uMsg) {
	case WM_GETMAINWINDOW:
		// ���ϼ����ڲ�ѯ�����ھ��
		return (LRESULT)GetParent(hwnd);
	case WM_SIZE:
		// �����б���ͼ��С����Ӧ��������
		if (hWndListView) {
			RECT rc;
			GetClientRect(hwnd, &rc);
			MoveWindow(hWndListView,
				rc.left + 20,          // ��ƫ��
				rc.top + 60,         // ��ƫ��
				rc.right - 40,       // ���
				rc.bottom - 80,      // �߶�
				TRUE);
		}
		break;
	case WM_NOTIFY:
		// �����б���ͼ֪ͨ��Ϣ
		switch (((LPNMHDR)lParam)->code) {
		case LVN_ITEMCHANGED: {
			LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
			if (pnmlv->uNewState & LVIS_SELECTED) {
				// ��Ŀ��ѡ��
				wchar_t szText[256] = { 0 };
				LVITEMW lvi = { 0 };
				lvi.mask = LVIF_TEXT;
				lvi.iItem = pnmlv->iItem;
				lvi.iSubItem = 0;
				lvi.pszText = szText;
				lvi.cchTextMax = 256;
				//��һ������
				SendMessageW(hWndListView, LVM_GETITEMW, 0, (LPARAM)&lvi);
				// ͨ�������Զ�����Ϣ��ȡ�����ھ��
				HWND mainWindow = (HWND)SendMessage(hwnd, WM_GETMAINWINDOW, 0, 0);
				if (mainWindow) {
					// ת����ť�����Ϣ��������
					SendMessage(mainWindow, WM_COMMAND, 7002, (LPARAM)&szText[0]);
				}
			}
			return 0;
		}
		}
		break;
	}

	// ������Ϣ����ԭʼ���ڹ��̴���
	return CallWindowProcW(originalProc, hwnd, uMsg, wParam, lParam);
}


// ��������
HWND createOverviewWindow(HINSTANCE hInstance, struct TabWindowsInfo *tabWindowsInfo, HWND hostWindow) {
	SendMessageW(hostWindow, WM_SETFONT, (WPARAM)(tabWindowsInfo->editorFontHandle), 0);

	hWndListView = CreateWindow(
		WC_LISTVIEW,
		L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS | LVS_SHOWSELALWAYS,
		50, 100, 550, 300, // ��ʼλ�úʹ�С
		hostWindow,
		(HMENU)7001, // �ؼ�ID
		hInstance,
		NULL
	);

	if (!hWndListView) {
		MessageBoxW(NULL, L"�޷������б���ͼ", L"����", MB_OK | MB_ICONERROR);
		return NULL;
	}
	// �����б���ͼ��չ��ʽ
	ListView_SetExtendedListViewStyle(hWndListView,
		LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// ��ʼ���б���ͼ��
	InitializeListViewColumns(hWndListView);

	WCHAR app1[256] = { 0 };
	WCHAR app2[256] = { 0 };
	//TCHAR app1[MAX_PATH] = { 0 };
	GetPrivateProfileStringW(L"Program", L"app1", L"", app1, MAX_PATH, IniName);
	GetPrivateProfileStringW(L"Program", L"app2", L"", app2, MAX_PATH, IniName);

	int i = 0;
	// ���ʾ������
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
	AddListViewItem(hWndListView, i, L"��Դ������", 1);
	AddListViewItem(hWndListView, i, L"����", 2);
	AddListViewItem(hWndListView, i, L"1", 3);
	i++;

	AddListViewItem(hWndListView, i, L"..\\..\\ProxyUI", 0);
	AddListViewItem(hWndListView, i, L"ProxyUI", 1);
	AddListViewItem(hWndListView, i, L"����", 2);
	AddListViewItem(hWndListView, i, L"1", 3);
	i++;

	AddListViewItem(hWndListView, i, L"C:\\Program Files\\FileZilla FTP Client\\filezilla.exe", 0);
	AddListViewItem(hWndListView, i, L"filezilla", 1);
	AddListViewItem(hWndListView, i, L"����", 2);
	AddListViewItem(hWndListView, i, L"1", 3);
	i++;

	AddListViewItem(hWndListView, i, L"C:\\HA_EmEditor9x64\\emed106c64\\EmEditor10x64\\EmEditor.exe", 0);
	AddListViewItem(hWndListView, i, L"EmEditor", 1);
	AddListViewItem(hWndListView, i, L"����", 2);
	AddListViewItem(hWndListView, i, L"1", 3);
	i++;

	AddListViewItem(hWndListView, i, L"cmd", 0);
	AddListViewItem(hWndListView, i, L"������", 1);
	AddListViewItem(hWndListView, i, L"����", 2);
	AddListViewItem(hWndListView, i, L"1", 3);

	// ���໯��������
	WNDPROC originalProc = (WNDPROC)SetWindowLongPtrW(hostWindow, GWLP_WNDPROC, (LONG_PTR)HostWindowProc);
	// �洢ԭʼ���ڹ��̣����ں�������
	SetWindowLongPtrW(hostWindow, GWLP_USERDATA, (LONG_PTR)originalProc);

	return hostWindow;
}

// ��ʼ���б���ͼ�У���ʽʹ�ÿ��ַ��汾��
void InitializeListViewColumns(HWND hWndListView) {
	LVCOLUMNW lvc = { 0 }; // ʹ�ÿ��ַ��汾�Ľṹ��
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; // �Ƴ������ LVIF_TEXT

	// ��1�У�
	lvc.iSubItem = 0;
	lvc.cx = 500;
	lvc.pszText = (LPWSTR)L"����";
	SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);


	// ��2�У�
	lvc.iSubItem = 1;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"���";
	SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);

	// ��3�У�
	lvc.iSubItem = 2;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"����";
	SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 2, (LPARAM)&lvc);

	// ��4�У�
	lvc.iSubItem = 3;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"����ctl+space";
	SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 3, (LPARAM)&lvc);

	//֧��sftp��winscp��
}

// ����б����ʽʹ�ÿ��ַ��汾��
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
		// ��������
		SendMessageW(hWndListView, LVM_SETITEMW, 0, (LPARAM)&lvi);

	}
}