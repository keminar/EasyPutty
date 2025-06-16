#pragma once

#include "apputils.h"
#include "enum.h"
#include "overview.h"
#include "EasyPuTTY.h"

static HINSTANCE g_appInstance;
static TabWindowsInfo* g_tabWindowsInfo = NULL;

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

void GetPuttySessionsPath(wchar_t* buffer, size_t bufferSize) {
	GetCurrentDirectoryPath(buffer, bufferSize);
	// 追加目标路径
	wcscat_s(buffer, bufferSize, L"config\\putty\\sessions");
}

void GetPuttyCredentialPath(wchar_t* buffer, size_t bufferSize) {
	GetCurrentDirectoryPath(buffer, bufferSize);
	wcscat_s(buffer, bufferSize, L"config\\putty\\credential");
}

void GetProgramPath(wchar_t* buffer, size_t bufferSize) {
	GetCurrentDirectoryPath(buffer, bufferSize);
	wcscat_s(buffer, bufferSize, L"config\\program");
}

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
	if (!show) {
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE; // 隐藏窗口
	}

	// 进程信息
	PROCESS_INFORMATION pi = { 0 };

	// 创建新进程
	if (CreateProcess(
		appPath,            // 程序路径
		NULL,               // 命令行参数
		NULL,               // 进程安全属性
		NULL,               // 线程安全属性
		FALSE,              // 不继承句柄
		CREATE_NO_WINDOW,   // 关键标志：创建无窗口的进程
		NULL,               // 环境变量
		NULL,               // 当前目录
		&si,                // 启动信息
		&pi                 // 进程信息
	)) {

		// 关闭进程和线程句柄
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		if (!show) {
			MessageBoxW(NULL, L"启动成功", L"提示", MB_OK);
		}
		return 0;
	}
	else {
		MessageBoxW(NULL, L"启动失败", L"提示", MB_OK);
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
	case WM_INITDIALOG:
		createEnum(g_appInstance, g_tabWindowsInfo, hDlg);
		return (INT_PTR)TRUE;

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
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

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

// 查找当前选择的会话行
BOOL FindSelectedSession(wchar_t* name, int nameLen) {
	// 当前标签
	HWND tabCtrlWinHandle = (g_tabWindowsInfo)->tabCtrlWinHandle;
	int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
	if (sel != -1) {
		TCCUSTOMITEM tabCtrlItemInfo = getTabItemInfo(tabCtrlWinHandle, sel);
		// 在overview标签上
		if (!tabCtrlItemInfo.attachWindowHandle) {
			wchar_t szType[256] = { 0 };
			HWND hListView = GetDlgItem(tabCtrlItemInfo.hostWindowHandle, ID_LIST_VIEW);
			if (!hListView) {
				return FALSE;
			}
			int selectedItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
			if (selectedItem != -1) {
				ListView_GetItemText(hListView, selectedItem, 1, szType, sizeof(szType));
				if (wcsstr(szType, L"PuTTY") != NULL) {
					ListView_GetItemText(hListView, selectedItem, 0, name, nameLen);
					return TRUE;
				}
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
			PathAddExtension(iniPath, L".ini");   // 添加 .ini 扩展
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
		if (LOWORD(wParam) == ID_LIST_REFRESH) {
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
		}
		else if (LOWORD(wParam) == IDOK)
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

			if (name[0] == L'\0') {
				MessageBoxW(hDlg, L"简称为必填", L"错误", MB_OK);
				return FALSE;
			}
			if (hostname[0] == L'\0') {
				MessageBoxW(hDlg, L"地址为必填", L"错误", MB_OK);
				return FALSE;
			}
			// 构建路径
			GetPuttySessionsPath(dirPath, MAX_PATH);
			CreateDirectoryIfNotExists(dirPath);
			PathCombine(iniPath, dirPath, name);  // 合并目录和文件名
			PathAddExtension(iniPath, L".ini");   // 添加 .ini 扩展

			BOOL result = WritePrivateProfileString(SECTION_NAME, L"Name", name, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"HostName", hostname, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Port", port, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"ConnectType", connectType, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Credential", credential, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Tags", tags, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"OtherParams", otherParams, iniPath);
			if (!result) {
				showError(hDlg, L"添加失败");
				return FALSE;
			}

			// 当前标签
			HWND tabCtrlWinHandle = g_tabWindowsInfo->tabCtrlWinHandle;
			int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
			if (sel != -1) {
				TCCUSTOMITEM tabCtrlItemInfo = getTabItemInfo(tabCtrlWinHandle, sel);
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


// 设置管理
INT_PTR CALLBACK SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG: {
		wchar_t iniPath[MAX_PATH] = { 0 };
		wchar_t putty[MAX_PATH] = { 0 };
		wchar_t winscp[MAX_PATH] = { 0 };
		wchar_t filezilla[MAX_PATH] = { 0 };
		wchar_t params[MAX_PATH] = { 0 };
		wchar_t puttygen[MAX_PATH] = { 0 };
		wchar_t pageant[MAX_PATH] = { 0 };
		HWND hEdit;

		GetAppIni(iniPath, MAX_PATH);
		GetPrivateProfileStringW(SECTION_NAME, L"Putty", L"", putty, MAX_PATH, iniPath);
		GetPrivateProfileStringW(SECTION_NAME, L"Winscp", L"", winscp, MAX_PATH, iniPath);
		GetPrivateProfileStringW(SECTION_NAME, L"Filezilla", L"", filezilla, MAX_PATH, iniPath);
		GetPrivateProfileStringW(SECTION_NAME, L"Putty_params", L"", params, MAX_PATH, iniPath);
		GetPrivateProfileStringW(SECTION_NAME, L"Puttygen", L"", puttygen, MAX_PATH, iniPath);
		GetPrivateProfileStringW(SECTION_NAME, L"Pageant", L"", pageant, MAX_PATH, iniPath);

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

			//使用相对路径受GetOpenFileName影响，所以要用全路径
			GetAppIni(iniPath, MAX_PATH);

			BOOL result = WritePrivateProfileString(SECTION_NAME, L"Putty", putty, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Winscp", winscp, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Filezilla", filezilla, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Putty_params", params, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Puttygen", puttygen, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Pageant", pageant, iniPath);
			if (!result) {
				showError(hDlg, L"添加失败");
				return FALSE;
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_PUTTY) {
			setBrowser(hDlg, L"可执行文件\0*.exe\0所有文件\0*.*\0", IDC_PUTTY);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_WINSCP) {
			setBrowser(hDlg, L"可执行文件\0*.exe\0所有文件\0*.*\0", IDC_WINSCP);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_FILEZILLA) {
			setBrowser(hDlg, L"可执行文件\0*.exe\0所有文件\0*.*\0", IDC_FILEZILLA);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_PUTTYGEN) {
			setBrowser(hDlg, L"可执行文件\0*.exe\0所有文件\0*.*\0", IDC_PUTTYGEN);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_PAGEANT) {
			setBrowser(hDlg, L"可执行文件\0*.exe\0所有文件\0*.*\0", IDC_PAGEANT);
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
	swprintf_s(fullError, 512, L"%s！错误代码: %lu\n%ls", showMsg, errorCode, errorMsg);
	MessageBox(hwnd, fullError, L"错误", MB_ICONERROR);
}
// 认证凭证
INT_PTR CALLBACK CredentialProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
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
				MessageBoxW(hDlg, L"名称为必填", L"错误", MB_OK);
				return FALSE;
			}
			// 构建路径
			GetPuttyCredentialPath(dirPath, MAX_PATH);
			CreateDirectoryIfNotExists(dirPath);
			PathCombine(iniPath, dirPath, name);  // 合并目录和文件名
			PathAddExtension(iniPath, L".ini");   // 添加 .ini 扩展

			BOOL result = WritePrivateProfileString(SECTION_NAME, L"Name", name, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"UserName", username, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Password", password, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"PrivateKey", ppkfile, iniPath);

			if (!result) {
				showError(hDlg, L"添加失败");
				return FALSE;
			}
			HWND parent = GetParent(hDlg);
			SendMessage(parent, WM_COMMAND, ID_LIST_REFRESH, NULL);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER) {
			setBrowser(hDlg, L"PuTTY私钥.ppk\0*.ppk\0所有文件\0*.*\0", IDC_PPK);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
