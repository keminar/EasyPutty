#pragma once

#include "resource.h"

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