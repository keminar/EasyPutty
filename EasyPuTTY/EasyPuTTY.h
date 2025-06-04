#pragma once

#include "resource.h"

#define IDC_TABCONTROL 100

HWND hTabCtrl;
HWND hToolbar;
HWND g_mainWindowHandle;
int g_tabHitIndex;

// will use this structure to group fields which describe tab header and editor
struct TabEditorsInfo {
	int tabWindowIdentifier;
	int tabIncrementor;
	HWND parentWinHandle;
	HWND tabCtrlWinHandle;
	HMENU tabMenuHandle;
	LOGFONT editorFontProperties;
	HFONT editorFontHandle;
};

// Data associated with each tab control item. We will use it instead of TCITEM. First member must be TCITEMHEADER, other members we can freely define
typedef struct tagTCCUSTOMITEM {
	TCITEMHEADER tcitemheader;
	HWND editorWindowHandle;
	HWND puttyWindowHandle;
	wchar_t* fileName;
} TCCUSTOMITEM;

// single global instance of TabEditorsInfo
struct TabEditorsInfo g_tabEditorsInfo;

void CreateToolBarTabControl(struct TabEditorsInfo *tabEditorsInfo, HWND parentWinHandle);
void AddNewTab(HWND hTab);
void RemoveTab(HWND hTab, int currentTab);
LRESULT processTabNotification(HWND tabCtrlWinHandle, HMENU tabMenuHandle, HWND menuCommandProcessorWindowHandle, int code);

void selectTab(HWND tabCtrlWinHandle, int tabIndex);
void moveTabToPosition(struct TabEditorsInfo* tabEditorsInfo, int tabIndex, int newPosition);
void selectedTabToRightmost();
void selectedTabToRight();
void selectedTabToLeftmost();
void selectedTabToLeft();