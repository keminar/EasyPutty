#pragma once

#include "apputils.h"
#include "enum.h"
#include "overview.h"
#include "EasyPuTTY.h"

static HINSTANCE g_appInstance;
static TabWindowsInfo* g_tabWindowsInfo = NULL;

// 全局变量用于存储当前快捷键
wchar_t g_hotkeyString[256] = { 0 };
HHOOK g_hookInput = NULL;
HWND g_hEditInputHotkey = NULL; // 存储编辑框句柄的全局变量

// 递归创建目录
BOOL CreateDirectoryRecursiveW(LPCWSTR lpPath) {
	if (!lpPath || !*lpPath) return FALSE;

	WCHAR path[MAX_PATH];
	wcscpy_s(path, lpPath);

	// 转换斜杠并处理每个组件
	for (WCHAR* p = path; *p; p++) {
		if (*p == L'/' || *p == L'\\') {
			*p = L'\\';  // 统一为反斜杠

			// 跳过连续斜杠
			if (p > path && *(p - 1) == L'\\') continue;
			// 跳过驱动器号后的冒号 (如 "C:")
			if (p == path + 2 && *(p - 1) == L':') continue;

			// 临时截断路径
			WCHAR orig = *p;
			*p = L'\0';
			// 创建目录（忽略已存在错误）
			if (!CreateDirectoryW(path, NULL)) {
				DWORD err = GetLastError();
				if (err != ERROR_ALREADY_EXISTS) {
					*p = orig;  // 恢复路径
					return FALSE;
				}
			}

			*p = orig;  // 恢复路径
		}
	}

	// 创建最终目录
	return CreateDirectoryW(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

// 检查目录是否存在，不存在则创建
BOOL CreateDirectoryIfNotExists(LPCTSTR lpPathName) {
	DWORD dwAttrib = GetFileAttributes(lpPathName);

	// 如果目录不存在
	if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
		return CreateDirectoryRecursiveW(lpPathName) ||
			(GetLastError() == ERROR_ALREADY_EXISTS);
	}

	// 如果路径存在，判断是否为目录
	return (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

// 获取当前可执行文件的完整路径
void GetCurrentDirectoryPath(wchar_t* buffer, size_t bufferSize) {
	GetModuleFileNameW(NULL, buffer, bufferSize);

	// 查找最后一个反斜杠
	wchar_t* lastSlash = wcsrchr(buffer, L'\\');
	if (lastSlash) {
		// 截断路径，只保留目录部分
		*(lastSlash + 1) = L'\0';
	}
}

// putty配置文件目录
void GetPuttySessionsPath(wchar_t* buffer, size_t bufferSize) {
	GetCurrentDirectoryPath(buffer, bufferSize);
	// 追加目标路径
	wcscat_s(buffer, bufferSize, L"config\\putty\\sessions");
}

// 凭证目录
void GetPuttyCredentialPath(wchar_t* buffer, size_t bufferSize) {
	GetCurrentDirectoryPath(buffer, bufferSize);
	wcscat_s(buffer, bufferSize, L"config\\putty\\credential");
}

// 其他程序目录
void GetProgramPath(wchar_t* buffer, size_t bufferSize) {
	GetCurrentDirectoryPath(buffer, bufferSize);
	wcscat_s(buffer, bufferSize, L"config\\program");
}

// 主配置
void GetAppIni(wchar_t* buffer, size_t bufferSize) {
	GetCurrentDirectoryPath(buffer, bufferSize);
	wcscat_s(buffer, bufferSize, L"EasyPuTTY.ini");
}

// 安全截断宽字符字符串并添加省略号
void TruncateString(const wchar_t* src, wchar_t* dest, size_t maxLength) {
	size_t srcLen = wcslen(src);

	// 如果源字符串长度小于等于最大长度，直接复制
	if (srcLen <= maxLength) {
		wcscpy_s(dest, srcLen + 1, src);
		return;
	}

	// 计算截断位置（留出省略号的空间）
	size_t truncatePos = maxLength - 3;  // 减去 3 为省略号留出空间

	// 检查截断位置是否在代理对中间
	if (truncatePos > 0 &&
		src[truncatePos - 1] >= 0xD800 && src[truncatePos - 1] <= 0xDBFF &&  // 高代理项
		src[truncatePos] >= 0xDC00 && src[truncatePos] <= 0xDFFF) {           // 低代理项
		truncatePos--;  // 调整截断位置以避免破坏代理对
	}

	// 复制截断的字符串
	wcsncpy_s(dest, maxLength + 1, src, truncatePos);

	// 添加省略号
	wcscat_s(dest, maxLength + 1, L"...");
}


// 启动进程
int startApp(const wchar_t* appPath, BOOL show) {
	// 进程启动信息
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	// 进程信息
	PROCESS_INFORMATION pi = { 0 };
	if (!show) {
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE; // 隐藏窗口
	}
	BOOL ret = CreateProcessW(
		NULL,
		(LPWSTR)appPath,               // 程序路径
		NULL,                          // 命令行参数（若需传递参数，需单独构造）
		NULL,                          // 进程安全属性
		FALSE,                         // 不继承句柄
		show ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW, // 根据显示选项设置标志
		NULL,                          // 环境变量（使用父进程环境）
		NULL,                          // 当前目录（使用父进程当前目录）
		&si,                           // 启动信息
		&pi                            // 进程信息
	);
	// 创建新进程
	if (ret) {
		// 等待进程初始化完成
		WaitForInputIdle(pi.hProcess, INFINITE);

		// 关闭进程和线程句柄
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return 0;
	}
	else {
		wchar_t msgCaption[MAX_PATH] = { 0 };
		wcscpy_s(msgCaption, _countof(msgCaption), GetString(IDS_MESSAGE_CAPTION));
		MessageBoxW(NULL, GetString(IDS_PROCESS_START_FAIL), msgCaption, MB_OK);
		return 1;
	}
}

// 显示弹窗
INT_PTR showDialogBox(HINSTANCE appInstance, TabWindowsInfo* tabWindowsInfo, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc)
{
	g_appInstance = appInstance;
	g_tabWindowsInfo = tabWindowsInfo;
	return DialogBox(appInstance, lpTemplateName, hWndParent, lpDialogFunc);
}

// “枚举”框的消息处理程序。
INT_PTR CALLBACK ENUMProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG: {
		SetWindowText(GetDlgItem(hDlg, IDOK), GetString(IDS_BTN_OK));
		SetWindowText(GetDlgItem(hDlg, IDCANCEL), GetString(IDS_BTN_CANCEL));
		SetWindowText(GetDlgItem(hDlg, IDC_REFRESH), GetString(IDS_LIST_REFRESH));
		createEnum(g_appInstance, g_tabWindowsInfo, hDlg);
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			HWND hListView = GetDlgItem(hDlg, ID_ENUM_VIEW);
			int selectedItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);

			// 检索HWND
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


// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG: {
		SetWindowText(GetDlgItem(hDlg, IDOK), GetString(IDS_BTN_OK));
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	case WM_NOTIFY: {
		LPNMHDR pNMHDR = (LPNMHDR)lParam;
		if (pNMHDR->code == NM_CLICK || pNMHDR->code == NM_RETURN) {
			// 检查是否是Syslink控件
			if (pNMHDR->hwndFrom == GetDlgItem(hDlg, IDC_SYSLINK1) || pNMHDR->hwndFrom == GetDlgItem(hDlg, IDC_SYSLINK2)) {
				// 处理链接点击
				PNMLINK pNMLink = (PNMLINK)lParam;
				wchar_t szUrl[MAX_PATH];
				wcscpy_s(szUrl, MAX_PATH, pNMLink->item.szUrl);

				// 调用系统默认浏览器打开URL
				ShellExecute(NULL, L"open", szUrl, NULL, NULL, SW_SHOWNORMAL);
				return 0;
			}
		}
		break;
	}
	}
	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK Pageant(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG: {
		// 在创建新进程前，检查Pageant是否已运行
		HWND existingHwnd = FindWindow(NULL, L"Pageant");
		HWND status = GetDlgItem(hDlg, IDC_PAGEANT_STATUS);
		if (existingHwnd == NULL) {
			SetWindowText(status, GetString(IDS_PROCESS_STARTING));
			wchar_t iniPath[MAX_PATH] = { 0 };
			wchar_t pageant[MAX_PATH] = { 0 };
			// putty路径
			GetAppIni(iniPath, MAX_PATH);
			GetPrivateProfileStringW(SECTION_NAME, L"Pageant", L"", pageant, MAX_PATH, iniPath);
			if (pageant[0] == L'\0') {
				wcscpy_s(pageant, MAX_PATH, L".\\pageant.exe");
			}
			startApp(pageant, FALSE);
			for (int i = 0; i < 50 && !existingHwnd; i++) {
				existingHwnd = FindWindow(NULL, L"Pageant");
				Sleep(100);
			}
			if (!existingHwnd) {
				SetWindowText(status, GetString(IDS_PROCESS_NOT_STARTED));
			}
			else {
				SetWindowText(status, GetString(IDS_PROCESS_STARTED));
			}
		}
		else {
			SetWindowText(status, GetString(IDS_PROCESS_STARTED));
		}
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// 查找当前选择的会话行
BOOL FindSelectedSession(wchar_t* name, int nameLen) {
	// 当前标签
	HWND tabCtrlWinHandle = (g_tabWindowsInfo)->tabCtrlWinHandle;
	int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
	if (sel != -1) {
		TCCUSTOMITEM tabCtrlItemInfo = { 0 };
		getTabItemInfo(tabCtrlWinHandle, sel, &tabCtrlItemInfo);
		// 在overview标签上
		if (!tabCtrlItemInfo.attachWindowHandle) {
			wchar_t szType[256] = { 0 };
			HWND hListView = GetDlgItem(tabCtrlItemInfo.hostWindowHandle, ID_LIST_VIEW);
			if (!hListView) {
				return FALSE;
			}
			int selectedItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
			if (selectedItem != -1) {
				ListView_GetItemText(hListView, selectedItem, 0, name, nameLen);
				return TRUE;
			}
		}
	}
	return FALSE;
}

// 会话管理
INT_PTR CALLBACK SessionProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG: {
		HWND hComboBox, credentialComboBox;
		wchar_t** credentialFileList = NULL;
		wchar_t credentialPath[MAX_PATH] = { 0 };
		int credentialCount = 0;
		CredentialInfo credentialConfig = { 0 };
		wchar_t dirPath[MAX_PATH] = { 0 };
		wchar_t iniPath[MAX_PATH] = { 0 };
		SessionInfo sessionConfig = { 0 };
		HWND hEdit;
		wchar_t findName[MAX_PATH] = { 0 };
		wchar_t port[20];

		hComboBox = GetDlgItem(hDlg, IDC_SESSION_CONNECT);
		// 添加选项
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)L"SSHv2");
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)L"SSHv1");
		SendMessage(hComboBox, CB_SETCURSEL, 0, 0);

		credentialComboBox = GetDlgItem(hDlg, IDC_SESSION_CREDENTIAL);
		GetPuttyCredentialPath(credentialPath, MAX_PATH);
		credentialFileList = ListIniFiles(credentialPath, &credentialCount);
		for (int i = 0; i < credentialCount; i++) {
			if (credentialFileList[i] != NULL) {
				ReadCredentialFromIni(credentialFileList[i], &credentialConfig);
				SendMessage(credentialComboBox, CB_ADDSTRING, 0, (LPARAM)credentialConfig.name);
			}
		}

		// 查找当前选择的会话行简称
		BOOL ret = FindSelectedSession(findName, MAX_PATH);
		if (ret) {
			// 构建路径
			GetPuttySessionsPath(dirPath, MAX_PATH);
			PathCombine(iniPath, dirPath, findName);  // 合并目录和文件名
			swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//强制增加后缀
			ReadSessionFromIni(iniPath, &sessionConfig);
			swprintf(port, 20, L"%d", sessionConfig.port);

			hEdit = GetDlgItem(hDlg, IDC_SESSION_NAME);
			SetWindowText(hEdit, sessionConfig.name);
			hEdit = GetDlgItem(hDlg, IDC_SESSION_IP);
			SetWindowText(hEdit, sessionConfig.hostName);
			hEdit = GetDlgItem(hDlg, IDC_SESSION_PORT);
			SetWindowText(hEdit, port);
			hEdit = GetDlgItem(hDlg, IDC_SESSION_CONNECT);
			SetWindowText(hEdit, sessionConfig.connectType);
			hEdit = GetDlgItem(hDlg, IDC_SESSION_CREDENTIAL);
			SetWindowText(hEdit, sessionConfig.credential);
			hEdit = GetDlgItem(hDlg, IDC_TAGS);
			SetWindowText(hEdit, sessionConfig.tags);
			hEdit = GetDlgItem(hDlg, IDC_OTHER_PARAMS);
			SetWindowText(hEdit, sessionConfig.otherParams);
		}

		return (INT_PTR)TRUE;
	}

	case WM_COMMAND:
		if (LOWORD(wParam) == ID_LIST_REFRESH) {//刷新凭证下拉
			HWND hEdit = GetDlgItem(hDlg, IDC_SESSION_CREDENTIAL);
			wchar_t credential[MAX_PATH] = { 0 };
			if (lParam){
				wcscpy_s(credential, MAX_PATH, (wchar_t*)lParam);
			}
			if (credential[0] == '\0') {
				// 上次值
				GetWindowText(hEdit, credential, MAX_PATH);
			}

			HWND credentialComboBox;
			wchar_t** credentialFileList = NULL;
			wchar_t credentialPath[MAX_PATH] = { 0 };
			int credentialCount = 0;
			CredentialInfo credentialConfig = { 0 };

			credentialComboBox = GetDlgItem(hDlg, IDC_SESSION_CREDENTIAL);
			if (credentialComboBox) {
				ComboBox_ResetContent(credentialComboBox);
				GetPuttyCredentialPath(credentialPath, MAX_PATH);
				credentialFileList = ListIniFiles(credentialPath, &credentialCount);
				for (int i = 0; i < credentialCount; i++) {
					if (credentialFileList[i] != NULL) {
						ReadCredentialFromIni(credentialFileList[i], &credentialConfig);
						SendMessage(credentialComboBox, CB_ADDSTRING, 0, (LPARAM)credentialConfig.name);
					}
				}
			}
			SetWindowText(hEdit, credential);
		}
		else if (LOWORD(wParam) == IDOK)//保存
		{
			wchar_t dirPath[MAX_PATH] = { 0 };
			wchar_t iniPath[MAX_PATH] = { 0 };
			wchar_t name[MAX_PATH] = { 0 };
			wchar_t hostname[MAX_PATH] = { 0 };
			wchar_t port[20] = { 0 };
			wchar_t connectType[MAX_PATH] = { 0 };
			wchar_t credential[MAX_PATH] = { 0 };
			wchar_t tags[MAX_PATH] = { 0 };
			wchar_t otherParams[MAX_PATH] = { 0 };
			HWND hEdit;

			hEdit = GetDlgItem(hDlg, IDC_SESSION_NAME);
			GetWindowText(hEdit, name, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_SESSION_IP);
			GetWindowText(hEdit, hostname, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_SESSION_PORT);
			GetWindowText(hEdit, port, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_SESSION_CONNECT);
			GetWindowText(hEdit, connectType, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_SESSION_CREDENTIAL);
			GetWindowText(hEdit, credential, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_TAGS);
			GetWindowText(hEdit, tags, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_OTHER_PARAMS);
			GetWindowText(hEdit, otherParams, MAX_PATH);

			wchar_t msgCaption[MAX_PATH] = { 0 };
			wcscpy_s(msgCaption, _countof(msgCaption), GetString(IDS_MESSAGE_CAPTION));
			if (name[0] == L'\0') {
				MessageBoxW(hDlg, GetString(IDS_NAME_REQUIRED), msgCaption, MB_OK);
				return FALSE;
			}
			if (hostname[0] == L'\0') {
				MessageBoxW(hDlg, GetString(IDS_HOSTNAME_REQUIRED), msgCaption, MB_OK);
				return FALSE;
			}
			// 构建路径
			GetPuttySessionsPath(dirPath, MAX_PATH);
			CreateDirectoryIfNotExists(dirPath);
			PathCombine(iniPath, dirPath, name);  // 合并目录和文件名
			swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//强制增加后缀

			BOOL result = WritePrivateProfileString(SECTION_NAME, L"Name", name, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"HostName", hostname, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Port", port, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"ConnectType", connectType, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Credential", credential, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Tags", tags, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"OtherParams", otherParams, iniPath);
			if (!result) {
				showError(hDlg, GetString(IDS_ADD_FAIL));
				return FALSE;
			}

			// 给当前标签发送刷新
			HWND tabCtrlWinHandle = g_tabWindowsInfo->tabCtrlWinHandle;
			int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
			if (sel != -1) {
				TCCUSTOMITEM tabCtrlItemInfo = { 0 };
				getTabItemInfo(tabCtrlWinHandle, sel, &tabCtrlItemInfo);
				// 在overview标签上
				if (!tabCtrlItemInfo.attachWindowHandle) {
					SendMessage(tabCtrlItemInfo.hostWindowHandle, WM_COMMAND, ID_LIST_REFRESH, NULL);
				}
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_CREDENTIAL_ADD) {//添加凭证
			DialogBox(g_appInstance, MAKEINTRESOURCE(IDD_CREDENTIAL), hDlg, CredentialProc);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// 判断是否为修饰键
bool IsModifierKey(WPARAM vkCode) {
	return (vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL ||
		vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT ||
		vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU ||
		vkCode == VK_LWIN || vkCode == VK_RWIN);
}

// 获取按键名称的辅助函数
const wchar_t* GetKeyName(WPARAM vkCode) {
	switch (vkCode) {
	case VK_LCONTROL: return L"Ctrl";
	case VK_RCONTROL: return L"Ctrl";
	case VK_LSHIFT:   return L"Shift";
	case VK_RSHIFT:   return L"Shift";
	case VK_LMENU:    return L"Alt";
	case VK_RMENU:    return L"Alt";
	case VK_LWIN:     return L"Win";
	case VK_RWIN:     return L"Win";
	case VK_LEFT:     return L"Left";
	case VK_UP:       return L"Up";
	case VK_RIGHT:    return L"Right";
	case VK_DOWN:     return L"Down";
	case VK_RETURN:   return L"Enter";
	case VK_ESCAPE:   return L"Esc";
	case VK_SPACE:    return L"Space";
	case VK_TAB:      return L"Tab";
	case VK_BACK:     return L"Backspace";
	case VK_DELETE:   return L"Delete";
	case VK_INSERT:   return L"Insert";
	case VK_HOME:     return L"Home";
	case VK_END:      return L"End";
	case VK_PRIOR:    return L"Page Up";
	case VK_NEXT:     return L"Page Down";
	case VK_F1:       return L"F1";
	case VK_F2:       return L"F2";
	case VK_F3:       return L"F3";
	case VK_F4:       return L"F4";
	case VK_F5:       return L"F5";
	case VK_F6:       return L"F6";
	case VK_F7:       return L"F7";
	case VK_F8:       return L"F8";
	case VK_F9:       return L"F9";
	case VK_F10:      return L"F10";
	case VK_F11:      return L"F11";
	case VK_F12:      return L"F12";
	default:          return NULL;
	}
}

// 根据名称获取虚拟键码
// 根据宽字符名称获取虚拟键码
WORD GetKeyVk(const wchar_t* name) {
	if (wcscmp(name, L"CTRL") == 0) return VK_CONTROL;
	if (wcscmp(name, L"SHIFT") == 0) return VK_SHIFT;
	if (wcscmp(name, L"ALT") == 0) return VK_MENU;
	if (wcscmp(name, L"WIN") == 0) return VK_LWIN;
	if (wcscmp(name, L"LEFT") == 0) return VK_LEFT;
	if (wcscmp(name, L"UP") == 0) return VK_UP;
	if (wcscmp(name, L"RIGHT") == 0) return VK_RIGHT;
	if (wcscmp(name, L"DOWN") == 0) return VK_DOWN;
	if (wcscmp(name, L"ENTER") == 0) return VK_RETURN;
	if (wcscmp(name, L"ESC") == 0) return VK_ESCAPE;
	if (wcscmp(name, L"SPACE") == 0) return VK_SPACE;
	if (wcscmp(name, L"TAB") == 0) return VK_TAB;
	if (wcscmp(name, L"BACKSPACE") == 0) return VK_BACK;
	if (wcscmp(name, L"DELETE") == 0) return VK_DELETE;
	if (wcscmp(name, L"INSERT") == 0) return VK_INSERT;
	if (wcscmp(name, L"HOME") == 0) return VK_HOME;
	if (wcscmp(name, L"END") == 0) return VK_END;
	if (wcscmp(name, L"PAGEUP") == 0) return VK_PRIOR;
	if (wcscmp(name, L"PAGEDOWN") == 0) return VK_NEXT;
	if (wcscmp(name, L"F1") == 0) return VK_F1;
	if (wcscmp(name, L"F2") == 0) return VK_F2;
	if (wcscmp(name, L"F3") == 0) return VK_F3;
	if (wcscmp(name, L"F4") == 0) return VK_F4;
	if (wcscmp(name, L"F5") == 0) return VK_F5;
	if (wcscmp(name, L"F6") == 0) return VK_F6;
	if (wcscmp(name, L"F7") == 0) return VK_F7;
	if (wcscmp(name, L"F8") == 0) return VK_F8;
	if (wcscmp(name, L"F9") == 0) return VK_F9;
	if (wcscmp(name, L"F10") == 0) return VK_F10;
	if (wcscmp(name, L"F11") == 0) return VK_F11;
	if (wcscmp(name, L"F12") == 0) return VK_F12;
	return 0;
}

// 解析快捷键字符串
BOOL parseShortcut(const wchar_t* shortcut, WORD* vkList, int* count) {
	wchar_t temp[256] = { 0 };
	wcsncpy_s(temp, shortcut, _TRUNCATE);

	wchar_t* context = NULL;
	wchar_t* token = wcstok_s(temp, L"+", &context);
	*count = 0;

	while (token != NULL && *count < MAX_KEYS) {
		// 转换为大写
		for (int i = 0; token[i]; i++) {
			token[i] = towupper(token[i]);
		}

		// 尝试查找预定义键名
		WORD vk = GetKeyVk(token);
		if (vk != 0) {
			vkList[(*count)++] = vk;
		}
		else {
			// 处理单个字符键
			if (wcslen(token) == 1) {
				char ansiChar[2] = { 0 };
				WideCharToMultiByte(CP_ACP, 0, token, 1, ansiChar, 1, NULL, NULL);
				vk = VkKeyScanA(ansiChar[0]) & 0xFF;
				vkList[(*count)++] = vk;
			}
			else {
				return FALSE;
			}
		}

		token = wcstok_s(NULL, L"+", &context);
	}

	return (*count > 0);
}


// 模拟按键
void simulateKeys(WORD* vkList, int count) {
	INPUT inputs[MAX_KEYS * 2] = { 0 };
	int inputCount = 0;

	// 按下所有按键
	for (int i = 0; i < count; i++) {
		inputs[inputCount].type = INPUT_KEYBOARD;
		inputs[inputCount].ki.wVk = vkList[i];
		inputCount++;
	}

	// 释放所有按键（顺序相反）
	for (int i = count - 1; i >= 0; i--) {
		inputs[inputCount].type = INPUT_KEYBOARD;
		inputs[inputCount].ki.wVk = vkList[i];
		inputs[inputCount].ki.dwFlags = KEYEVENTF_KEYUP;
		inputCount++;
	}

	// 发送输入
	SendInput(inputCount, inputs, sizeof(INPUT));
}

// 发送输入法切换
void sendInputHotkey() {
	wchar_t shortcut[256] = { 0 };// 长度和录制时一样
	WORD vkList[MAX_KEYS];
	int keyCount = 0;

	// 读取INI文件
	wchar_t iniPath[MAX_PATH] = { 0 };
	GetAppIni(iniPath, MAX_PATH);
	GetPrivateProfileStringW(SECTION_NAME, L"Input_hotkey", L"", shortcut, 256, iniPath);

	// 没设置快捷键
	if (shortcut[0] == L'\0') {
		return;
	}

	// 解析快捷键
	if (!parseShortcut(shortcut, vkList, &keyCount)) {
		return;
	}

	// 模拟按键
	simulateKeys(vkList, keyCount);
}

// 低级键盘钩子处理函数
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	static bool modifierStates[256] = { 0 }; // 跟踪每个按键的状态

	if (nCode >= 0) {
		KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
		WPARAM vkCode = kb->vkCode;

		// 检查当前焦点是否在目标 Edit Control 中
		// 获取当前有焦点的窗口
		HWND focusedWnd = GetForegroundWindow();
		if (focusedWnd) {
			// 获取焦点窗口中的子控件（如果有）
			HWND focusedCtrl = GetFocus();
			// 判断焦点是否在目标 Edit 控件（g_hEdit）
			if (focusedCtrl != g_hEditInputHotkey) {
				// 焦点不在目标 Edit，不处理，直接交给系统
				return CallNextHookEx(g_hookInput, nCode, wParam, lParam);
			}
		}
		else {
			// 无焦点窗口，不处理
			return CallNextHookEx(g_hookInput, nCode, wParam, lParam);
		}

		// 更新按键状态
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			modifierStates[vkCode] = true;
		}
		else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
			modifierStates[vkCode] = false;
		}

		// 只处理按键按下事件
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			// 清空之前的快捷键字符串
			memset(g_hotkeyString, 0, sizeof(g_hotkeyString));

			// 检测所有可能的修饰键状态，避免重复计算
			bool ctrl = modifierStates[VK_LCONTROL] || modifierStates[VK_RCONTROL];
			bool shift = modifierStates[VK_LSHIFT] || modifierStates[VK_RSHIFT];
			bool alt = modifierStates[VK_LMENU] || modifierStates[VK_RMENU];
			bool win = modifierStates[VK_LWIN] || modifierStates[VK_RWIN];

			// 构建修饰键部分（按 Ctrl+Shift+Alt+Win 顺序）
			if (ctrl) wcscat_s(g_hotkeyString, _countof(g_hotkeyString), L"Ctrl+");
			if (shift) wcscat_s(g_hotkeyString, _countof(g_hotkeyString), L"Shift+");
			if (alt) wcscat_s(g_hotkeyString, _countof(g_hotkeyString), L"Alt+");
			if (win) wcscat_s(g_hotkeyString, _countof(g_hotkeyString), L"Win+");

			// 如果是修饰键本身被按下，且没有其他修饰键，则只显示该修饰键
			if (IsModifierKey(vkCode)) {
				// 检查是否有其他同类型的修饰键被按下
				bool hasOtherCtrl = ctrl && (vkCode != VK_LCONTROL && vkCode != VK_RCONTROL);
				bool hasOtherShift = shift && (vkCode != VK_LSHIFT && vkCode != VK_RSHIFT);
				bool hasOtherAlt = alt && (vkCode != VK_LMENU && vkCode != VK_RMENU);
				bool hasOtherWin = win && (vkCode != VK_LWIN && vkCode != VK_RWIN);

				// 如果没有其他同类型的修饰键，则只显示该修饰键
				if (!hasOtherCtrl && !hasOtherShift && !hasOtherAlt && !hasOtherWin) {
					const wchar_t* keyName = GetKeyName(vkCode);
					if (keyName) {
						wcscpy_s(g_hotkeyString, _countof(g_hotkeyString), keyName);
					}
				}
			}
			// 否则获取按键名称
			else {
				const wchar_t* keyName = GetKeyName(vkCode);
				wchar_t keyNameBuffer[128] = { 0 };

				if (keyName) {
					// 使用预定义的按键名称
					wcscpy_s(keyNameBuffer, _countof(keyNameBuffer), keyName);
				}
				else {
					// 对于字母键，显示大写形式
					if (vkCode >= 'A' && vkCode <= 'Z') {
						keyNameBuffer[0] = (wchar_t)vkCode;
					}
					// 对于数字键，直接显示数字
					else if (vkCode >= '0' && vkCode <= '9') {
						keyNameBuffer[0] = (wchar_t)vkCode;
					}
					// 其他键使用系统函数获取名称
					else {
						UINT scanCode = kb->scanCode;
						GetKeyNameTextW(scanCode << 16, keyNameBuffer, _countof(keyNameBuffer));

						// 如果获取失败，使用虚拟键码的十六进制表示
						if (wcslen(keyNameBuffer) == 0) {
							swprintf_s(keyNameBuffer, _countof(keyNameBuffer), L"0x%02X", (UINT)vkCode);
						}
					}
				}

				// 添加按键名称
				wcscat_s(g_hotkeyString, _countof(g_hotkeyString), keyNameBuffer);
			}

			// 如果最后一个字符是加号，则移除它
			size_t len = wcslen(g_hotkeyString);
			if (len > 0 && g_hotkeyString[len - 1] == L'+') {
				g_hotkeyString[len - 1] = L'\0';
			}

			// 更新编辑框文本
			if (g_hEditInputHotkey) SendMessageW(g_hEditInputHotkey, WM_SETTEXT, 0, (LPARAM)g_hotkeyString);

			// 拦截所有按键事件
			return 1;
		}
	}

	// 其他消息交给系统处理
	return CallNextHookEx(g_hookInput, nCode, wParam, lParam);
}

// 设置管理
INT_PTR CALLBACK SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool hotkeyDisabled = false;
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		// 在对话框销毁时卸载钩子
	case WM_DESTROY:
		if (g_hookInput) {
			UnhookWindowsHookEx(g_hookInput);
			g_hookInput = NULL;
		}
		break;
	case WM_INITDIALOG: {
		// 安装键盘钩子
		g_hookInput = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleW(NULL), 0);
		/*if (!g_hookInput) {
			MessageBoxW(hDlg, L"无法安装键盘钩子!", L"错误", MB_ICONERROR);
		}*/

		wchar_t iniPath[MAX_PATH] = { 0 };
		wchar_t putty[MAX_PATH] = { 0 };
		wchar_t winscp[MAX_PATH] = { 0 };
		wchar_t filezilla[MAX_PATH] = { 0 };
		wchar_t params[MAX_PATH] = { 0 };
		wchar_t puttygen[MAX_PATH] = { 0 };
		wchar_t pageant[MAX_PATH] = { 0 };
		wchar_t psftp[MAX_PATH] = { 0 };
		wchar_t inputHotkey[MAX_PATH] = { 0 };
		HWND hEdit;

		GetAppIni(iniPath, MAX_PATH);
		GetPrivateProfileStringW(SECTION_NAME, L"Putty", L"", putty, MAX_PATH, iniPath);
		GetPrivateProfileStringW(SECTION_NAME, L"Winscp", L"", winscp, MAX_PATH, iniPath);
		GetPrivateProfileStringW(SECTION_NAME, L"Filezilla", L"", filezilla, MAX_PATH, iniPath);
		GetPrivateProfileStringW(SECTION_NAME, L"Putty_params", L"", params, MAX_PATH, iniPath);
		GetPrivateProfileStringW(SECTION_NAME, L"Puttygen", L"", puttygen, MAX_PATH, iniPath);
		GetPrivateProfileStringW(SECTION_NAME, L"Pageant", L"", pageant, MAX_PATH, iniPath);
		GetPrivateProfileStringW(SECTION_NAME, L"Psftp", L"", psftp, MAX_PATH, iniPath);
		GetPrivateProfileStringW(SECTION_NAME, L"Input_hotkey", L"", inputHotkey, MAX_PATH, iniPath);

		hEdit = GetDlgItem(hDlg, IDC_PUTTY);
		SetWindowText(hEdit, putty);
		hEdit = GetDlgItem(hDlg, IDC_WINSCP);
		SetWindowText(hEdit, winscp);
		hEdit = GetDlgItem(hDlg, IDC_FILEZILLA);
		SetWindowText(hEdit, filezilla);
		hEdit = GetDlgItem(hDlg, IDC_PUTTY_PARAMS);
		SetWindowText(hEdit, params);
		hEdit = GetDlgItem(hDlg, IDC_PUTTYGEN);
		SetWindowText(hEdit, puttygen);
		hEdit = GetDlgItem(hDlg, IDC_PAGEANT);
		SetWindowText(hEdit, pageant);
		hEdit = GetDlgItem(hDlg, IDC_PSFTP);
		SetWindowText(hEdit, psftp);

		hEdit = GetDlgItem(hDlg, IDC_INPUT);
		SetWindowText(hEdit, inputHotkey);
		g_hEditInputHotkey = hEdit;
		return (INT_PTR)TRUE;
	}


	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			wchar_t iniPath[MAX_PATH] = { 0 };
			wchar_t putty[MAX_PATH] = { 0 };
			wchar_t winscp[MAX_PATH] = { 0 };
			wchar_t filezilla[MAX_PATH] = { 0 };
			wchar_t params[MAX_PATH] = { 0 };
			wchar_t puttygen[MAX_PATH] = { 0 };
			wchar_t pageant[MAX_PATH] = { 0 };
			wchar_t psftp[MAX_PATH] = { 0 };
			wchar_t inputHotkey[MAX_PATH] = { 0 };
			HWND hEdit;

			hEdit = GetDlgItem(hDlg, IDC_PUTTY);
			GetWindowText(hEdit, putty, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_WINSCP);
			GetWindowText(hEdit, winscp, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_FILEZILLA);
			GetWindowText(hEdit, filezilla, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_PUTTY_PARAMS);
			GetWindowText(hEdit, params, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_PUTTYGEN);
			GetWindowText(hEdit, puttygen, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_PAGEANT);
			GetWindowText(hEdit, pageant, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_PSFTP);
			GetWindowText(hEdit, psftp, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_INPUT);
			GetWindowText(hEdit, inputHotkey, MAX_PATH);

			//使用相对路径受GetOpenFileName影响，所以要用全路径
			GetAppIni(iniPath, MAX_PATH);

			BOOL result = WritePrivateProfileString(SECTION_NAME, L"Putty", putty, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Winscp", winscp, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Filezilla", filezilla, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Putty_params", params, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Puttygen", puttygen, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Pageant", pageant, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Psftp", psftp, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Input_hotkey", inputHotkey, iniPath);
			if (!result) {
				showError(hDlg, GetString(IDS_ADD_FAIL));
				return FALSE;
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_CLEAN) {
			HWND hEdit = GetDlgItem(hDlg, IDC_INPUT);
			SetWindowText(hEdit, L"");
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_PUTTY) {
			setBrowser(hDlg, GetString(IDS_BROWSER_EXE), IDC_PUTTY);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_WINSCP) {
			setBrowser(hDlg, GetString(IDS_BROWSER_COM), IDC_WINSCP);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_FILEZILLA) {
			setBrowser(hDlg, GetString(IDS_BROWSER_EXE), IDC_FILEZILLA);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_PUTTYGEN) {
			setBrowser(hDlg, GetString(IDS_BROWSER_EXE), IDC_PUTTYGEN);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_PAGEANT) {
			setBrowser(hDlg, GetString(IDS_BROWSER_EXE), IDC_PAGEANT);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_PSFTP) {
			setBrowser(hDlg, GetString(IDS_BROWSER_EXE), IDC_PSFTP);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// 点击按钮后弹出文件选择对话框
void setBrowser(HWND hwnd, LPCWSTR lpstrFilter, int nIDDlgItem)
{
	OPENFILENAME ofn = { 0 };
	wchar_t szFile[MAX_PATH] = { 0 };

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
	ofn.lpstrFilter = lpstrFilter;
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	//GetOpenFileName 对话框默认会将当前工作目录（CWD）更改为用户最后选择的目录
	//所以后续使用相对路径（如 .\EasyPuTTY.ini）时，文件会被写入到非预期位置。
	//所以写ini文件要使用全路径
	if (GetOpenFileName(&ofn)) {
		// 获取选择的文件路径并设置到编辑控件中
		HWND hEdit = GetDlgItem(hwnd, nIDDlgItem);
		SetWindowText(hEdit, szFile);
	}
}

void showError(HWND hwnd, const wchar_t* showMsg)
{
	// 获取错误代码
	DWORD errorCode = GetLastError();
	wchar_t errorMsg[256] = { 0 };

	// 格式化错误信息
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		errorMsg,
		256,
		NULL
	);

	// 显示错误信息
	wchar_t fullError[512];
	swprintf_s(fullError, 512, GetString(IDS_ERROR_FORMAT), showMsg, errorCode, errorMsg);
	MessageBox(hwnd, fullError, GetString(IDS_MESSAGE_CAPTION), MB_ICONERROR);
}

// 认证凭证
INT_PTR CALLBACK CredentialProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG: {
		HWND listBox;
		wchar_t** credentialFileList = NULL;
		wchar_t credentialPath[MAX_PATH] = { 0 };
		int credentialCount = 0;
		CredentialInfo credentialConfig = { 0 };

		listBox = GetDlgItem(hDlg, IDC_LIST_NAME);
		if (listBox) {
			ListBox_ResetContent(listBox);
			GetPuttyCredentialPath(credentialPath, MAX_PATH);
			credentialFileList = ListIniFiles(credentialPath, &credentialCount);
			for (int i = 0; i < credentialCount; i++) {
				if (credentialFileList[i] != NULL) {
					ReadCredentialFromIni(credentialFileList[i], &credentialConfig);
					SendMessage(listBox, LB_ADDSTRING, 0, (LPARAM)credentialConfig.name);
				}
			}
		}
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDC_ADD)
		{
			wchar_t dirPath[MAX_PATH] = { 0 };
			wchar_t iniPath[MAX_PATH] = { 0 };
			wchar_t name[MAX_PATH] = { 0 };
			wchar_t username[MAX_PATH] = { 0 };
			wchar_t password[MAX_PATH] = { 0 };
			wchar_t ppkfile[MAX_PATH] = { 0 };
			HWND hEdit;

			hEdit = GetDlgItem(hDlg, IDC_NAME);
			GetWindowText(hEdit, name, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_USERNAME);
			GetWindowText(hEdit, username, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_PASSWORD);
			GetWindowText(hEdit, password, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_PPK);
			GetWindowText(hEdit, ppkfile, MAX_PATH);

			if (name[0] == L'\0') {
				wchar_t msgCaption[MAX_PATH] = { 0 };
				wcscpy_s(msgCaption, _countof(msgCaption), GetString(IDS_MESSAGE_CAPTION));
				MessageBoxW(hDlg, GetString(IDS_NAME_REQUIRED), msgCaption, MB_OK);
				return FALSE;
			}
			// 构建路径
			GetPuttyCredentialPath(dirPath, MAX_PATH);
			CreateDirectoryIfNotExists(dirPath);
			PathCombine(iniPath, dirPath, name);  // 合并目录和文件名
			swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//强制增加后缀

			BOOL result = WritePrivateProfileString(SECTION_NAME, L"Name", name, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"UserName", username, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Password", password, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"PrivateKey", ppkfile, iniPath);

			if (!result) {
				showError(hDlg, GetString(IDS_ADD_FAIL));
				return FALSE;
			}
			if (LOWORD(wParam) == IDC_ADD) {
				HWND hListBox = GetDlgItem(hDlg, IDC_LIST_NAME);
				int count = SendMessageW(hListBox, LB_GETCOUNT, 0, 0);
				BOOL exist = FALSE;
				for (int i = 0; i < count; i++)
				{
					wchar_t existingText[256];
					SendMessageW(hListBox, LB_GETTEXT, i, (LPARAM)existingText);
					if (wcscmp(existingText, name) == 0) {
						exist = TRUE;
						break;
					}
				}
				if (!exist) {
					SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)name);
				}
				return FALSE;
			}
			else {
				// 父窗口可能是 连接配置表单，刷新下拉
				HWND parent = GetParent(hDlg);
				SendMessage(parent, WM_COMMAND, ID_LIST_REFRESH, (LPARAM)name);
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_CLEAR) {//清空名称
			HWND hEdit;
			hEdit = GetDlgItem(hDlg, IDC_NAME);
			SetWindowText(hEdit, L"");
			return (INT_PTR) FALSE;
		}
		else if (LOWORD(wParam) == IDC_RESET) {//重置
			HWND hEdit;
			hEdit = GetDlgItem(hDlg, IDC_NAME);
			SetWindowText(hEdit, L"");
			hEdit = GetDlgItem(hDlg, IDC_USERNAME);
			SetWindowText(hEdit, L"");
			hEdit = GetDlgItem(hDlg, IDC_PASSWORD);
			SetWindowText(hEdit, L"");
			hEdit = GetDlgItem(hDlg, IDC_PPK);
			SetWindowText(hEdit, L"");
			return (INT_PTR)FALSE;
		}
		else if (LOWORD(wParam) == IDC_DEL) {// 删除选中项
			wchar_t dirPath[MAX_PATH] = { 0 };
			wchar_t iniPath[MAX_PATH] = { 0 };

			HWND hListBox = GetDlgItem(hDlg, IDC_LIST_NAME);
			int selIndex = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
			if (selIndex != LB_ERR)
			{
				wchar_t itemText[256];
				SendMessage(hListBox, LB_GETTEXT, selIndex, (LPARAM)itemText);
				GetPuttyCredentialPath(dirPath, MAX_PATH);
				PathCombine(iniPath, dirPath, itemText);  // 合并目录和文件名
				swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//强制增加后缀
				DeleteFile(iniPath);
				SendMessage(hListBox, LB_DELETESTRING, selIndex, 0);
			}
			else
			{
				wchar_t msgCaption[MAX_PATH] = { 0 };
				wcscpy_s(msgCaption, _countof(msgCaption), GetString(IDS_MESSAGE_CAPTION));
				MessageBox(hDlg, GetString(IDS_BROWSER_REQUIRED), msgCaption, MB_ICONINFORMATION);
			}
			return (INT_PTR)FALSE;
		}
		else if (LOWORD(wParam) == IDC_EDIT) {//编辑
			wchar_t dirPath[MAX_PATH] = { 0 };
			wchar_t iniPath[MAX_PATH] = { 0 };
			HWND hListBox = GetDlgItem(hDlg, IDC_LIST_NAME);
			// 处理列表项选择变化
			int selIndex = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
			if (selIndex != LB_ERR)
			{
				wchar_t itemText[256];
				CredentialInfo config;
				HWND hEdit;
				SendMessage(hListBox, LB_GETTEXT, selIndex, (LPARAM)itemText);
				// 构建路径
				GetPuttyCredentialPath(dirPath, MAX_PATH);
				PathCombine(iniPath, dirPath, itemText);  // 合并目录和文件名
				swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//强制增加后缀
				ReadCredentialFromIni(iniPath, &config);

				hEdit = GetDlgItem(hDlg, IDC_NAME);
				SetWindowText(hEdit, config.name);
				hEdit = GetDlgItem(hDlg, IDC_USERNAME);
				SetWindowText(hEdit, config.userName);
				hEdit = GetDlgItem(hDlg, IDC_PASSWORD);
				SetWindowText(hEdit, config.password);
				hEdit = GetDlgItem(hDlg, IDC_PPK);
				SetWindowText(hEdit, config.privateKey);
			}
			return (INT_PTR)FALSE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER) {
			setBrowser(hDlg, GetString(IDS_BROWSER_PPK), IDC_PPK);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// 快捷应用
INT_PTR CALLBACK ProgramProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG: {
		wchar_t dirPath[MAX_PATH] = { 0 };
		wchar_t iniPath[MAX_PATH] = { 0 };
		ProgramInfo programConfig = { 0 };
		HWND hEdit;
		wchar_t findName[MAX_PATH] = { 0 };

		// 查找当前选择的会话行简称
		BOOL ret = FindSelectedSession(findName, MAX_PATH);
		if (ret) {
			// 构建路径
			GetProgramPath(dirPath, MAX_PATH);
			PathCombine(iniPath, dirPath, findName);  // 合并目录和文件名
			swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//强制增加后缀
			ReadProgramFromIni(iniPath, &programConfig);

			hEdit = GetDlgItem(hDlg, IDC_PRO_NAME);
			SetWindowText(hEdit, programConfig.name);
			hEdit = GetDlgItem(hDlg, IDC_PRO_PATH);
			SetWindowText(hEdit, programConfig.path);
			hEdit = GetDlgItem(hDlg, IDC_PRO_PARAM);
			SetWindowText(hEdit, programConfig.params);
			hEdit = GetDlgItem(hDlg, IDC_PRO_TAGS);
			SetWindowText(hEdit, programConfig.tags);
		}
		return (INT_PTR)TRUE;
	}

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			wchar_t dirPath[MAX_PATH] = { 0 };
			wchar_t iniPath[MAX_PATH] = { 0 };
			wchar_t name[MAX_PATH] = { 0 };
			wchar_t path[MAX_PATH] = { 0 };
			wchar_t params[MAX_PATH] = { 0 };
			wchar_t tags[MAX_PATH] = { 0 };
			HWND hEdit;

			hEdit = GetDlgItem(hDlg, IDC_PRO_NAME);
			GetWindowText(hEdit, name, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_PRO_PATH);
			GetWindowText(hEdit, path, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_PRO_PARAM);
			GetWindowText(hEdit, params, MAX_PATH);
			hEdit = GetDlgItem(hDlg, IDC_PRO_TAGS);
			GetWindowText(hEdit, tags, MAX_PATH);

			wchar_t msgCaption[MAX_PATH] = { 0 };
			wcscpy_s(msgCaption, _countof(msgCaption), GetString(IDS_MESSAGE_CAPTION));
			if (name[0] == L'\0') {
				MessageBoxW(hDlg, GetString(IDS_NAME_REQUIRED), msgCaption, MB_OK);
				return FALSE;
			}
			if (path[0] == L'\0') {
				MessageBoxW(hDlg, GetString(IDS_HOSTNAME_REQUIRED), msgCaption, MB_OK);
				return FALSE;
			}
			// 构建路径
			GetProgramPath(dirPath, MAX_PATH);
			CreateDirectoryIfNotExists(dirPath);
			PathCombine(iniPath, dirPath, name);  // 合并目录和文件名
			swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//强制增加后缀

			BOOL result = WritePrivateProfileString(SECTION_NAME, L"Name", name, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Path", path, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Params", params, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Tags", tags, iniPath);

			if (!result) {
				showError(hDlg, GetString(IDS_ADD_FAIL));
				return FALSE;
			}

			// 当前标签
			HWND tabCtrlWinHandle = g_tabWindowsInfo->tabCtrlWinHandle;
			int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
			if (sel != -1) {
				TCCUSTOMITEM tabCtrlItemInfo = { 0 };
				getTabItemInfo(tabCtrlWinHandle, sel, &tabCtrlItemInfo);
				// 在overview标签上
				if (!tabCtrlItemInfo.attachWindowHandle) {
					SendMessage(tabCtrlItemInfo.hostWindowHandle, WM_COMMAND, ID_LIST_REFRESH, NULL);
				}
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER) {
			setBrowser(hDlg, GetString(IDS_BROWSER_EXE), IDC_PRO_PATH);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// “重命名”框的消息处理程序。
INT_PTR CALLBACK RenameProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG: {
		HWND hEdit;
		wchar_t name[MAX_PATH] = { 0 };
		SendMessage(g_tabWindowsInfo->parentWinHandle, WM_COMMAND, ID_TAB_RENAME_GET, (LPARAM)&name);
		hEdit = GetDlgItem(hDlg, IDC_TAG_NAME);
		SetWindowText(hEdit, name);
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			wchar_t name[MAX_PATH] = { 0 };
			HWND hEdit;

			hEdit = GetDlgItem(hDlg, IDC_TAG_NAME);
			GetWindowText(hEdit, name, MAX_PATH);
			SendMessage(g_tabWindowsInfo->parentWinHandle, WM_COMMAND, ID_TAB_RENAME_DO, (LPARAM)name);
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
