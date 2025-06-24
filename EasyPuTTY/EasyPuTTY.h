#pragma once

#include "apputils.h"
#include "overview.h"
#include "attach.h"

// Data associated with each tab control item. We will use it instead of TCITEM. First member must be TCITEMHEADER, other members we can freely define
typedef struct tagTCCUSTOMITEM {
	TCITEMHEADER tcitemheader;
	HWND hostWindowHandle;
	HWND attachWindowHandle;
	DWORD attachProcessId;
	HANDLE processHandle;
	HANDLE waitHandle;
} TCCUSTOMITEM;


void CreateToolBarTabControl(struct TabWindowsInfo *tabWindowsInfo, HWND parentWinHandle);
int AddNewTab(HWND tabCtrlWinHandle, int suffix);
void AddNewOverview(struct TabWindowsInfo *tabWindowsInfo);
void RemoveTab(HWND tabCtrlWinHandle, int currentTab, BOOL quit);
LRESULT processTabNotification(HWND tabCtrlWinHandle, HMENU tabMenuHandle, HWND menuCommandProcessorWindowHandle, int code);

void moveTabToPosition(struct TabWindowsInfo *tabWindowsInfo, int tabIndex, int newPosition);
void selectedTabToRightmost();
void selectedTabToRight();
void selectedTabToLeftmost();
void selectedTabToLeft();

int GetTitleBarHeightWithoutMenu(HWND hWnd);
BOOL setTabWindowPos(HWND hostWinHandle, HWND attachWindowHandle, RECT rc, BOOL refresh);
void selectTab(HWND tabCtrlWinHandle, int tabIndex);
void showWindowForSelectedTabItem(HWND tabCtrlWinHandle, int selected);
TCCUSTOMITEM getTabItemInfo(HWND tabCtrlWinHandle, int i);
HWND getHostWindowForTabItem(HWND tabCtrlWinHandle, int i);
HRESULT resizeTabControl(struct TabWindowsInfo *tabWindowsInfo, RECT rc);
RECT getTabRect(struct TabWindowsInfo *tabWindowsInfo, RECT rc);
HWND createHostWindow(HINSTANCE hInstance, HWND parentWindow);
void AddAttachTab(struct TabWindowsInfo *tabWindowsInfo, HWND attachHwnd);
void DetachTab(HWND tabCtrlWinHandle, int indexTab);
HANDLE ProcessRegisterClose(DWORD dwThreadId, HANDLE* hWait);
void ProcessUnRegisterClose(HANDLE hWait, HANDLE hProcess);
void CALLBACK ProcessEndCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired);