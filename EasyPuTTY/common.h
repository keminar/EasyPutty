#pragma once

#include "resource.h"

// will use this structure to group fields which describe tab header and editor
struct TabWindowsInfo {
	int tabWindowIdentifier;//标签标识符IDC_TABCONTROL
	int tabIncrementor; //标签数自增
	HWND parentWinHandle; //父窗口g_mainWindowHandle;
	HWND tabCtrlWinHandle;//tabcontrol 句柄
	HMENU tabMenuHandle;//标签右键菜单
	LOGFONT editorFontProperties;//字体属性
	HFONT editorFontHandle;//逻辑字体句柄
};


// 定义自定义消息
#define WM_GETMAINWINDOW (WM_USER + 1)