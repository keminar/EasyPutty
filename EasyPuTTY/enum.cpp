#pragma once

#include "enum.h"
#include <vector>
#include <psapi.h>
#include <tlhelp32.h>
#include <vector>

// ���������
#pragma comment(lib, "psapi.lib")


// �洢������Ϣ�Ľṹ��
struct WindowInfo {
	HWND hWnd;
	wchar_t title[MAX_PATH];
	wchar_t className[MAX_PATH];
	DWORD processId;
	wchar_t processName[MAX_PATH];
	wchar_t processPath[MAX_PATH];
};

// ��ȡ�������ƺ�·��
BOOL GetProcessInfo(DWORD processId, wchar_t* processName, wchar_t* processPath, DWORD bufferSize) {
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	if (hProcess == NULL) {
		wcscpy_s(processName, bufferSize, L"�޷�����");
		wcscpy_s(processPath, bufferSize, L"�޷�����");
		return FALSE;
	}

	// ��ȡ��������
	if (GetModuleFileNameExW(hProcess, NULL, processPath, bufferSize)) {
		// ��ȡ�ļ���������·����
		wchar_t* fileName = wcsrchr(processPath, L'\\');
		if (fileName) {
			wcscpy_s(processName, bufferSize, fileName + 1);
		}
		else {
			wcscpy_s(processName, bufferSize, processPath);
		}
	}
	else {
		wcscpy_s(processName, bufferSize, L"δ֪");
		wcscpy_s(processPath, bufferSize, L"δ֪");
	}

	CloseHandle(hProcess);
	return TRUE;
}

// ����ö�ٻص�����
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
	// ��鴰���Ƿ�ɼ����б���
	if (!IsWindowVisible(hWnd))
		return TRUE;

	// ��ȡ���ڱ���
	wchar_t title[MAX_PATH] = { 0 };
	if (GetWindowTextW(hWnd, title, MAX_PATH) == 0)
		return TRUE;

	// ��ȡ��������
	wchar_t className[MAX_PATH] = { 0 };
	GetClassNameW(hWnd, className, MAX_PATH);

	// ��ȡ����ID
	DWORD processId;
	GetWindowThreadProcessId(hWnd, &processId);

	// ��ȡ�洢������Ϣ������
	std::vector<WindowInfo>* pWindows = reinterpret_cast<std::vector<WindowInfo>*>(lParam);

	// �洢������Ϣ
	WindowInfo info = { 0 };
	info.hWnd = hWnd;
	wcscpy_s(info.title, title);
	wcscpy_s(info.className, className);
	info.processId = processId;

	// ��ȡ�������ƺ�·��
	GetProcessInfo(processId, info.processName, info.processPath, MAX_PATH);

	pWindows->push_back(info);

	return TRUE; // ����ö��
}



// ��������
void createEnum(HINSTANCE hInstance, struct TabWindowsInfo *tabWindowsInfo, HWND hostWindow) {
	RECT rc;
	static std::vector<WindowInfo> windows;

	GetClientRect(hostWindow, &rc);
	HWND hListView = CreateWindowW(
		WC_LISTVIEW,
		L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS | LVS_SHOWSELALWAYS,
		rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top - 50, // ��ʼλ�úʹ�С
		hostWindow,
		(HMENU)ID_ENUM_VIEW, // �ؼ�ID
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
	InitEnumColumns(hListView);

	 
	// ö�����д���
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));

	// ��������Ϣ��ӵ�ListView
	wchar_t pidText[20];

	for (size_t i = 0; i < windows.size(); i++) {
		swprintf_s(pidText, L"%lu", windows[i].processId);
		AddEnumItem(hListView, i, pidText, 0);

		AddEnumItem(hListView, i, windows[i].title, 1);
		AddEnumItem(hListView, i, windows[i].processName, 2);
		AddEnumItem(hListView, i, windows[i].processPath, 3);
	}


	// ���໯��������
	//WNDPROC originalProc = (WNDPROC)SetWindowLongPtrW(hostWindow, GWLP_WNDPROC, (LONG_PTR)HostWindowProc);
	// �洢ԭʼ���ڹ��̣����ں�������
	//SetWindowLongPtrW(hostWindow, GWLP_USERDATA, (LONG_PTR)originalProc);
}

// ��ʼ���б���ͼ��
void InitEnumColumns(HWND hWndListView) {
	LVCOLUMNW lvc = { 0 }; // ʹ�ÿ��ַ��汾�Ľṹ��
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; // �Ƴ������ LVIF_TEXT

	// ��1�У�
	lvc.iSubItem = 0;
	lvc.cx = 100;
	lvc.pszText = (LPWSTR)L"PID";
	ListView_InsertColumn(hWndListView, 0, &lvc);
	//SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);


	// ��2�У�
	lvc.iSubItem = 1;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"����";
	ListView_InsertColumn(hWndListView, 1, &lvc);
	//SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);

	// ��3�У�
	lvc.iSubItem = 2;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"������";
	ListView_InsertColumn(hWndListView, 2, &lvc);
	//SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 2, (LPARAM)&lvc);

	// ��4�У�
	lvc.iSubItem = 3;
	lvc.cx = 400;
	lvc.pszText = (LPWSTR)L"·��";
	ListView_InsertColumn(hWndListView, 3, &lvc);
	//SendMessageW(hWndListView, LVM_INSERTCOLUMNW, 3, (LPARAM)&lvc);
}

// ����б���
void AddEnumItem(HWND hWndListView, int nItem, const wchar_t* pszText, int nSubItem) {
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