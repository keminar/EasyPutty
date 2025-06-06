#pragma once

#include "resource.h"
#include "common.h"
#include "overview.h"
#include "attach.h"

// Data associated with each tab control item. We will use it instead of TCITEM. First member must be TCITEMHEADER, other members we can freely define
typedef struct tagTCCUSTOMITEM {
	TCITEMHEADER tcitemheader;
	HWND overviewWindowHandle;
	HWND attachWindowHandle;
	DWORD attachProcessId;
} TCCUSTOMITEM;

void CreateToolBarTabControl(struct TabWindowsInfo *tabWindowsInfo, HWND parentWinHandle);
int AddNewTab(HWND tabCtrlWinHandle, int suffix);
int AddNewOverview(struct TabWindowsInfo *tabWindowsInfo);
void RemoveTab(HWND tabCtrlWinHandle, int currentTab);
LRESULT processTabNotification(HWND tabCtrlWinHandle, HMENU tabMenuHandle, HWND menuCommandProcessorWindowHandle, int code);

void moveTabToPosition(struct TabWindowsInfo *tabWindowsInfo, int tabIndex, int newPosition);
void selectedTabToRightmost();
void selectedTabToRight();
void selectedTabToLeftmost();
void selectedTabToLeft();

int GetTitleBarHeightWithoutMenu(HWND hWnd);
BOOL setTabWindowPos(HWND overviewWinHandle, HWND attachWindowHandle, RECT rc);
void selectTab(HWND tabCtrlWinHandle, int tabIndex);
void showWindowForSelectedTabItem(HWND tabCtrlWinHandle, int selected);
TCCUSTOMITEM getTabItemInfo(HWND tabCtrlWinHandle, int i);
HWND getWindowForTabItem(HWND tabCtrlWinHandle, int i);
void FocusWindow(HWND hWnd);
HRESULT resizeTabControl(struct TabWindowsInfo *tabWindowsInfo, RECT rc);