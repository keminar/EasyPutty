#pragma once

#include "resource.h"
#include "framework.h"

// will use this structure to group fields which describe tab header and editor
struct TabWindowsInfo {
	int tabWindowIdentifier;//��ǩ��ʶ��IDC_TABCONTROL
	int tabIncrementor; //��ǩ������
	HWND parentWinHandle; //������g_mainWindowHandle;
	HWND tabCtrlWinHandle;//tabcontrol ���
	HMENU tabMenuHandle;//��ǩ�Ҽ��˵�
	LOGFONT editorFontProperties;//��������
	HFONT editorFontHandle;//�߼�������
};


// �����Զ�����Ϣ
#define WM_GETMAINWINDOW (WM_USER + 1)

void TruncateString(const wchar_t* src, wchar_t* dest, size_t maxLength);
INT_PTR showDialogBox(HINSTANCE appInstance, TabWindowsInfo* tabWindowsInfo, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc);
int startApp(const wchar_t* appPath, BOOL show);

INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ENUM(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK Session(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK Credential(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);