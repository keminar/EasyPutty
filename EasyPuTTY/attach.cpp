#pragma once

#include "attach.h"

DWORD  dwThreadId;
wchar_t commandLine[MAX_PATH];

// �ص����������ڲ���PuTTY����
BOOL CALLBACK EnumPuTTYWindows(HWND hwnd, LPARAM lParam) {
	WCHAR szTitle[256] = { 0 };
	GetWindowTextW(hwnd, szTitle, sizeof(szTitle) / sizeof(WCHAR));
	// �����ձ��ⴰ��
	if (wcscmp(szTitle, L"") == 0) {
		return TRUE;
	}
	if (wcsstr(szTitle, L"��Դ������") != NULL) {
		// ��Դ����������idƥ�䲻��,Ҳ������IsWindowVisible
		if (wcsstr(commandLine, L"explorer") != NULL) {
			*(HWND*)lParam = hwnd;
			return FALSE; // ֹͣö��
		}
	}
	else if (IsWindowVisible(hwnd)) {
		DWORD processId;
		// ��ȡ���� ID
		GetWindowThreadProcessId(hwnd, &processId);
		if (dwThreadId == processId) {
			*(HWND*)lParam = hwnd;
			return FALSE; // ֹͣö��
		}
	}
	return TRUE;
}

// ����PuTTY���ھ��
HWND FindPuttyWindow() {
	HWND hwnd = NULL;
	EnumWindows(EnumPuTTYWindows, (LPARAM)&hwnd);
	return hwnd;
}

HWND createPuttyWindow(HINSTANCE hInstance, HWND hostWindow, const wchar_t* puttyPath) {
	// ����PuTTY����
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;

	// ��������������Ϣ
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
		MessageBoxW(NULL, L"�޷���������", L"����", MB_OK | MB_ICONERROR);
		return NULL;
	}

	// �رս��̺��߳̾��������Ҫ��һ��������
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	dwThreadId = pi.dwProcessId;

	// �ȴ�PuTTY���ڴ�������Ҫ������ʱ��
	HWND puttyHwnd = NULL;
	for (int i = 0; i < 30 && !puttyHwnd; i++) {
		puttyHwnd = FindPuttyWindow();
		Sleep(100);
	}

	// ����PuTTY����
	if (!puttyHwnd) {
		return NULL;
	}
	// Ƕ��PuTTY���ڵ���������
	SetParent(puttyHwnd, hostWindow);

	// ����PuTTY������ʽ
	LONG_PTR style = GetWindowLongPtr(puttyHwnd, GWL_STYLE);
	style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

	// Ӧ������ʽ
	SetWindowLongPtr(puttyHwnd, GWL_STYLE, style);
	return puttyHwnd;
}
