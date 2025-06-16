#pragma once

#include "apputils.h"
#include "enum.h"

static HINSTANCE g_appInstance;
static TabWindowsInfo* g_tabWindowsInfo = NULL;

// ��ȫ�ضϿ��ַ��ַ��������ʡ�Ժ�
void TruncateString(const wchar_t* src, wchar_t* dest, size_t maxLength) {
	size_t srcLen = wcslen(src);

	// ���Դ�ַ�������С�ڵ�����󳤶ȣ�ֱ�Ӹ���
	if (srcLen <= maxLength) {
		wcscpy_s(dest, srcLen + 1, src);
		return;
	}

	// ����ض�λ�ã�����ʡ�ԺŵĿռ䣩
	size_t truncatePos = maxLength - 3;  // ��ȥ 3 Ϊʡ�Ժ������ռ�

	// ���ض�λ���Ƿ��ڴ�����м�
	if (truncatePos > 0 &&
		src[truncatePos - 1] >= 0xD800 && src[truncatePos - 1] <= 0xDBFF &&  // �ߴ�����
		src[truncatePos] >= 0xDC00 && src[truncatePos] <= 0xDFFF) {           // �ʹ�����
		truncatePos--;  // �����ض�λ���Ա����ƻ������
	}

	// ���ƽضϵ��ַ���
	wcsncpy_s(dest, maxLength + 1, src, truncatePos);

	// ���ʡ�Ժ�
	wcscat_s(dest, maxLength + 1, L"...");
}



int startApp(const wchar_t* appPath, BOOL show) {
	// ����������Ϣ
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	if (!show) {
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE; // ���ش���
	}

	// ������Ϣ
	PROCESS_INFORMATION pi = { 0 };

	// �����½���
	if (CreateProcess(
		appPath,            // ����·��
		NULL,               // �����в���
		NULL,               // ���̰�ȫ����
		NULL,               // �̰߳�ȫ����
		FALSE,              // ���̳о��
		CREATE_NO_WINDOW,   // �ؼ���־�������޴��ڵĽ���
		NULL,               // ��������
		NULL,               // ��ǰĿ¼
		&si,                // ������Ϣ
		&pi                 // ������Ϣ
	)) {

		// �رս��̺��߳̾��
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		if (!show) {
			MessageBoxW(NULL, L"�����ɹ�", L"��ʾ", MB_OK);
		}
		return 0;
	}
	else {
		MessageBoxW(NULL, L"����ʧ��", L"��ʾ", MB_OK);
		return 1;
	}
}

INT_PTR showDialogBox(HINSTANCE appInstance, TabWindowsInfo* tabWindowsInfo, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc)
{
	g_appInstance = appInstance;
	g_tabWindowsInfo = tabWindowsInfo;
	return DialogBox(appInstance, lpTemplateName, hWndParent, lpDialogFunc);
}

// ��ö�١������Ϣ�������
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

			// ����HWND
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


// �����ڡ������Ϣ�������
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
