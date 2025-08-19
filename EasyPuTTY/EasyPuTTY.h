#pragma once

#include "apputils.h"
#include "overview.h"
#include "attach.h"
#include "logs.h"
#include "lang_manager.h"

// Data associated with each tab control item. We will use it instead of TCITEM. First member must be TCITEMHEADER, other members we can freely define
typedef struct tagTCCUSTOMITEM {
	TCITEMHEADER tcitemheader;
	HWND hostWindowHandle;
	HWND attachWindowHandle;
	DWORD attachProcessId;
	HANDLE processHandle;
	HANDLE waitHandle;
	LPWSTR command;
} TCCUSTOMITEM;

void CreateToolBarTabControl(struct TabWindowsInfo *tabWindowsInfo, HWND parentWinHandle);
int AddNewTab(HWND tabCtrlWinHandle, int suffix);
int AddNewOverview(struct TabWindowsInfo *tabWindowsInfo);
void RemoveTab(HWND tabCtrlWinHandle, int currentTab, BOOL quit);
LRESULT processTabNotification(HWND tabCtrlWinHandle, HMENU tabMenuHandle, HWND menuCommandProcessorWindowHandle, int code);
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

void moveTabToPosition(struct TabWindowsInfo *tabWindowsInfo, int tabIndex, int newPosition);
void selectedTabToRightmost();
void selectedTabToRight();
void selectedTabToLeftmost();
void selectedTabToLeft();

int GetTitleBarHeightWithoutMenu(HWND hWnd);
BOOL setTabWindowPos(HWND hostWinHandle, HWND attachWindowHandle, RECT rc, BOOL refresh);
void selectTab(HWND tabCtrlWinHandle, int tabIndex);
void showWindowForSelectedTabItem(HWND tabCtrlWinHandle, int selected);
void getTabItemInfo(HWND tabCtrlWinHandle, int i, TCCUSTOMITEM* tabCtrlItemInfo);
HWND getHostWindowForTabItem(HWND tabCtrlWinHandle, int i);
HRESULT resizeTabControl(struct TabWindowsInfo *tabWindowsInfo, RECT rc);
RECT getTabRect(struct TabWindowsInfo *tabWindowsInfo, RECT rc);
HWND createHostWindow(HINSTANCE hInstance, HWND parentWindow);
void AddAttachTab(struct TabWindowsInfo *tabWindowsInfo, HWND attachHwnd);
void DetachTab(HWND tabCtrlWinHandle, int indexTab);
HANDLE ProcessRegisterClose(DWORD dwThreadId, HANDLE* hWait);
void ProcessUnRegisterClose(HANDLE hWait, HANDLE hProcess);
void CALLBACK ProcessEndCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired);
void openAttach(HWND tabCtrlWinHandle, int selected, wchar_t* name, wchar_t* command);
void PerformSearch(HWND hWnd);
int clipboardLen();
void registerAccel(HWND hWnd);
void unRegisterAccel(HWND hWnd);
void cloneTab(HWND tabCtrlWinHandle);

void createDebugWindow();
LRESULT CALLBACK DebugWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ToolbarProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);