#pragma once

#include "enum.h"
#include <psapi.h>
#include <tlhelp32.h>

// ���������
#pragma comment(lib, "psapi.lib")

// ȫ�ֱ���:
WCHAR myWindowClass[256];            // ����������
HWND enumWindow;                     // ��ǰ���ھ��

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
	// �ų�������
	if (enumWindow == hWnd)
		return TRUE;

	// �ų�����������
	wchar_t className[256] = { 0 };
	GetClassNameW(hWnd, className, ARRAYSIZE(className));
	if (wcsstr(className, myWindowClass) != NULL) {
		return TRUE;
	}

	// ��ȡ���ڱ���
	wchar_t title[MAX_PATH] = { 0 };
	if (GetWindowTextW(hWnd, title, sizeof(title) / sizeof(WCHAR)) == 0)
		return TRUE;

	// ��ȡ����ID
	DWORD processId;
	GetWindowThreadProcessId(hWnd, &processId);

	// ��ȡ�洢������Ϣ������
	WindowVector* pWindows = (WindowVector*)lParam;

	// �洢������Ϣ
	WindowInfo info = { 0 };
	info.hWnd = hWnd;
	wcscpy_s(info.title, title);
	info.processId = processId;

	// ��ȡ�������ƺ�·��
	GetProcessInfo(processId, info.processName, info.processPath, MAX_PATH);

	// ��֪��Ҫ�ŵ��Ľ���
	if (wcsstr(info.processName, L"explorer.exe") != NULL) {
		if (wcsstr(title, L"Program Manager") != NULL) {//�������
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
	else if (wcsstr(info.processName, L"WeMail.exe") != NULL) {//΢���ʼ�
		return TRUE;
	}
	/*else if (wcsstr(info.processName, L"�޷�����") != NULL) {
		return TRUE;
	}
	else if (wcsstr(info.processName, L"δ֪") != NULL) {//win7һЩ���̻�õ����
		return TRUE;
	}*/
	
	AddWindowVector(pWindows, &info);

	return TRUE; // ����ö��
}

// ��������
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
		rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top - 50, // ��ʼλ�úʹ�С
		parentWindow,
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

	// ����������
	LoadStringW(hInstance, IDC_EASYPUTTY, myWindowClass, sizeof(myWindowClass)/sizeof(wchar_t));
	 
	// ö�����д���
	windows = InitWindowVector(10);
	EnumWindows(EnumWindowsProc, (LPARAM)windows);

	// ��������Ϣ��ӵ�ListView
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

// ��ʼ���б���ͼ��
void InitEnumColumns(HWND hWndListView) {
	LVCOLUMNW lvc = { 0 };
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; 

	// ��1�У�
	lvc.iSubItem = 0;
	lvc.cx = 100;
	lvc.pszText = (LPWSTR)L"PID";
	ListView_InsertColumn(hWndListView, 0, &lvc);


	// ��2�У�
	lvc.iSubItem = 1;
	lvc.cx = 225;
	lvc.pszText = (LPWSTR)L"����";
	ListView_InsertColumn(hWndListView, 1, &lvc);

	// ��3�У�
	lvc.iSubItem = 2;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"������";
	ListView_InsertColumn(hWndListView, 2, &lvc);

	// ��4�У�
	lvc.iSubItem = 3;
	lvc.cx = 460;
	lvc.pszText = (LPWSTR)L"·��";
	ListView_InsertColumn(hWndListView, 3, &lvc);
}

// ����б���
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

// static ���ƺ������ļ��ɼ�������ͷ�ļ�����
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