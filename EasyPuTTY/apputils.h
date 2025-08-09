#pragma once

#include "resource.h"
#include "framework.h"
#include "lang_manager.h"

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

#define SECTION_NAME L"Settings"
// 定义自定义消息
#define WM_GETMAINWINDOW (WM_USER + 1)
// 快捷键长度
#define MAX_KEYS 16

void TruncateString(const wchar_t* src, wchar_t* dest, size_t maxLength);
INT_PTR showDialogBox(HINSTANCE appInstance, TabWindowsInfo* tabWindowsInfo, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc);
int startApp(const wchar_t* appPath, BOOL show);
void showError(HWND hwnd, const wchar_t* showMsg);
void setBrowser(HWND hwnd, LPCWSTR lpstrFilter, int nIDDlgItem);

void GetProgramPath(wchar_t* buffer, size_t bufferSize);
void GetAppIni(wchar_t* buffer, size_t bufferSize);
void GetCurrentDirectoryPath(wchar_t* buffer, size_t bufferSize);
void GetPuttySessionsPath(wchar_t* buffer, size_t bufferSize);
void GetPuttyCredentialPath(wchar_t* buffer, size_t bufferSize);
BOOL FindSelectedSession(wchar_t* name, int nameLen);
BOOL CreateDirectoryRecursiveW(LPCWSTR lpPath);
BOOL CreateDirectoryIfNotExists(LPCTSTR lpPathName);

INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ENUMProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SessionProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CredentialProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProgramProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK RenameProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK Pageant(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
const wchar_t* GetKeyName(WPARAM vkCode);
bool IsModifierKey(WPARAM vkCode);
WORD GetKeyVk(const wchar_t* name);
BOOL parseShortcut(const wchar_t* shortcut, WORD* vkList, int* count);
void simulateKeys(WORD* vkList, int count);
void sendInputHotkey();