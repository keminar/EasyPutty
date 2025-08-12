#pragma once

#include "apputils.h"
#include "enum.h"
#include "overview.h"
#include "EasyPuTTY.h"

static HINSTANCE g_appInstance;
static TabWindowsInfo* g_tabWindowsInfo = NULL;

// ȫ�ֱ������ڴ洢��ǰ��ݼ�
wchar_t g_hotkeyString[256] = { 0 };
HHOOK g_hookInput = NULL;
HWND g_hEditInputHotkey = NULL; // �洢�༭������ȫ�ֱ���

// �ݹ鴴��Ŀ¼
BOOL CreateDirectoryRecursiveW(LPCWSTR lpPath) {
	if (!lpPath || !*lpPath) return FALSE;

	WCHAR path[MAX_PATH];
	wcscpy_s(path, lpPath);

	// ת��б�ܲ�����ÿ�����
	for (WCHAR* p = path; *p; p++) {
		if (*p == L'/' || *p == L'\\') {
			*p = L'\\';  // ͳһΪ��б��

			// ��������б��
			if (p > path && *(p - 1) == L'\\') continue;
			// �����������ź��ð�� (�� "C:")
			if (p == path + 2 && *(p - 1) == L':') continue;

			// ��ʱ�ض�·��
			WCHAR orig = *p;
			*p = L'\0';
			// ����Ŀ¼�������Ѵ��ڴ���
			if (!CreateDirectoryW(path, NULL)) {
				DWORD err = GetLastError();
				if (err != ERROR_ALREADY_EXISTS) {
					*p = orig;  // �ָ�·��
					return FALSE;
				}
			}

			*p = orig;  // �ָ�·��
		}
	}

	// ��������Ŀ¼
	return CreateDirectoryW(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

// ���Ŀ¼�Ƿ���ڣ��������򴴽�
BOOL CreateDirectoryIfNotExists(LPCTSTR lpPathName) {
	DWORD dwAttrib = GetFileAttributes(lpPathName);

	// ���Ŀ¼������
	if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
		return CreateDirectoryRecursiveW(lpPathName) ||
			(GetLastError() == ERROR_ALREADY_EXISTS);
	}

	// ���·�����ڣ��ж��Ƿ�ΪĿ¼
	return (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

// ��ȡ��ǰ��ִ���ļ�������·��
void GetCurrentDirectoryPath(wchar_t* buffer, size_t bufferSize) {
	GetModuleFileNameW(NULL, buffer, bufferSize);

	// �������һ����б��
	wchar_t* lastSlash = wcsrchr(buffer, L'\\');
	if (lastSlash) {
		// �ض�·����ֻ����Ŀ¼����
		*(lastSlash + 1) = L'\0';
	}
}

// putty�����ļ�Ŀ¼
void GetPuttySessionsPath(wchar_t* buffer, size_t bufferSize) {
	GetCurrentDirectoryPath(buffer, bufferSize);
	// ׷��Ŀ��·��
	wcscat_s(buffer, bufferSize, L"config\\putty\\sessions");
}

// ƾ֤Ŀ¼
void GetPuttyCredentialPath(wchar_t* buffer, size_t bufferSize) {
	GetCurrentDirectoryPath(buffer, bufferSize);
	wcscat_s(buffer, bufferSize, L"config\\putty\\credential");
}

// ��������Ŀ¼
void GetProgramPath(wchar_t* buffer, size_t bufferSize) {
	GetCurrentDirectoryPath(buffer, bufferSize);
	wcscat_s(buffer, bufferSize, L"config\\program");
}

// ������
void GetAppIni(wchar_t* buffer, size_t bufferSize) {
	GetCurrentDirectoryPath(buffer, bufferSize);
	wcscat_s(buffer, bufferSize, L"EasyPuTTY.ini");
}

// ��ȫ�ضϿ��ַ��ַ��������ʡ�Ժ�
void TruncateString(const wchar_t* src, wchar_t* dest, size_t maxLength) {
	size_t srcLen = wcslen(src);

	// ���Դ�ַ�������С�ڵ�����󳤶ȣ�ֱ�Ӹ���
	if (srcLen <= maxLength) {
		wcscpy_s(dest, srcLen + 1, src);
		return;
	}

	// ����ض�λ�ã�����ʡ�ԺŵĿռ䣩
	size_t truncatePos = maxLength - 3;  // ��ȥ 3 Ϊʡ�Ժ������ռ�

	// ���ض�λ���Ƿ��ڴ�����м�
	if (truncatePos > 0 &&
		src[truncatePos - 1] >= 0xD800 && src[truncatePos - 1] <= 0xDBFF &&  // �ߴ�����
		src[truncatePos] >= 0xDC00 && src[truncatePos] <= 0xDFFF) {           // �ʹ�����
		truncatePos--;  // �����ض�λ���Ա����ƻ������
	}

	// ���ƽضϵ��ַ���
	wcsncpy_s(dest, maxLength + 1, src, truncatePos);

	// ���ʡ�Ժ�
	wcscat_s(dest, maxLength + 1, L"...");
}


// ��������
int startApp(const wchar_t* appPath, BOOL show) {
	// ����������Ϣ
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	// ������Ϣ
	PROCESS_INFORMATION pi = { 0 };
	if (!show) {
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE; // ���ش���
	}
	BOOL ret = CreateProcessW(
		NULL,
		(LPWSTR)appPath,               // ����·��
		NULL,                          // �����в��������贫�ݲ������赥�����죩
		NULL,                          // ���̰�ȫ����
		FALSE,                         // ���̳о��
		show ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW, // ������ʾѡ�����ñ�־
		NULL,                          // ����������ʹ�ø����̻�����
		NULL,                          // ��ǰĿ¼��ʹ�ø����̵�ǰĿ¼��
		&si,                           // ������Ϣ
		&pi                            // ������Ϣ
	);
	// �����½���
	if (ret) {
		// �ȴ����̳�ʼ�����
		WaitForInputIdle(pi.hProcess, INFINITE);

		// �رս��̺��߳̾��
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

// ��ʾ����
INT_PTR showDialogBox(HINSTANCE appInstance, TabWindowsInfo* tabWindowsInfo, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc)
{
	g_appInstance = appInstance;
	g_tabWindowsInfo = tabWindowsInfo;
	return DialogBox(appInstance, lpTemplateName, hWndParent, lpDialogFunc);
}

// ��ö�١������Ϣ�������
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

			// ����HWND
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


// �����ڡ������Ϣ�������
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
			// ����Ƿ���Syslink�ؼ�
			if (pNMHDR->hwndFrom == GetDlgItem(hDlg, IDC_SYSLINK1) || pNMHDR->hwndFrom == GetDlgItem(hDlg, IDC_SYSLINK2)) {
				// �������ӵ��
				PNMLINK pNMLink = (PNMLINK)lParam;
				wchar_t szUrl[MAX_PATH];
				wcscpy_s(szUrl, MAX_PATH, pNMLink->item.szUrl);

				// ����ϵͳĬ���������URL
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
		// �ڴ����½���ǰ�����Pageant�Ƿ�������
		HWND existingHwnd = FindWindow(NULL, L"Pageant");
		HWND status = GetDlgItem(hDlg, IDC_PAGEANT_STATUS);
		if (existingHwnd == NULL) {
			SetWindowText(status, GetString(IDS_PROCESS_STARTING));
			wchar_t iniPath[MAX_PATH] = { 0 };
			wchar_t pageant[MAX_PATH] = { 0 };
			// putty·��
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

// ���ҵ�ǰѡ��ĻỰ��
BOOL FindSelectedSession(wchar_t* name, int nameLen) {
	// ��ǰ��ǩ
	HWND tabCtrlWinHandle = (g_tabWindowsInfo)->tabCtrlWinHandle;
	int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
	if (sel != -1) {
		TCCUSTOMITEM tabCtrlItemInfo = { 0 };
		getTabItemInfo(tabCtrlWinHandle, sel, &tabCtrlItemInfo);
		// ��overview��ǩ��
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

// �Ự����
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
		// ���ѡ��
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

		// ���ҵ�ǰѡ��ĻỰ�м��
		BOOL ret = FindSelectedSession(findName, MAX_PATH);
		if (ret) {
			// ����·��
			GetPuttySessionsPath(dirPath, MAX_PATH);
			PathCombine(iniPath, dirPath, findName);  // �ϲ�Ŀ¼���ļ���
			swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//ǿ�����Ӻ�׺
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
		if (LOWORD(wParam) == ID_LIST_REFRESH) {//ˢ��ƾ֤����
			HWND hEdit = GetDlgItem(hDlg, IDC_SESSION_CREDENTIAL);
			wchar_t credential[MAX_PATH] = { 0 };
			if (lParam){
				wcscpy_s(credential, MAX_PATH, (wchar_t*)lParam);
			}
			if (credential[0] == '\0') {
				// �ϴ�ֵ
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
		else if (LOWORD(wParam) == IDOK)//����
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
			// ����·��
			GetPuttySessionsPath(dirPath, MAX_PATH);
			CreateDirectoryIfNotExists(dirPath);
			PathCombine(iniPath, dirPath, name);  // �ϲ�Ŀ¼���ļ���
			swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//ǿ�����Ӻ�׺

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

			// ����ǰ��ǩ����ˢ��
			HWND tabCtrlWinHandle = g_tabWindowsInfo->tabCtrlWinHandle;
			int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
			if (sel != -1) {
				TCCUSTOMITEM tabCtrlItemInfo = { 0 };
				getTabItemInfo(tabCtrlWinHandle, sel, &tabCtrlItemInfo);
				// ��overview��ǩ��
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
		else if (LOWORD(wParam) == IDC_CREDENTIAL_ADD) {//���ƾ֤
			DialogBox(g_appInstance, MAKEINTRESOURCE(IDD_CREDENTIAL), hDlg, CredentialProc);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// �ж��Ƿ�Ϊ���μ�
bool IsModifierKey(WPARAM vkCode) {
	return (vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL ||
		vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT ||
		vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU ||
		vkCode == VK_LWIN || vkCode == VK_RWIN);
}

// ��ȡ�������Ƶĸ�������
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

// �������ƻ�ȡ�������
// ���ݿ��ַ����ƻ�ȡ�������
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

// ������ݼ��ַ���
BOOL parseShortcut(const wchar_t* shortcut, WORD* vkList, int* count) {
	wchar_t temp[256] = { 0 };
	wcsncpy_s(temp, shortcut, _TRUNCATE);

	wchar_t* context = NULL;
	wchar_t* token = wcstok_s(temp, L"+", &context);
	*count = 0;

	while (token != NULL && *count < MAX_KEYS) {
		// ת��Ϊ��д
		for (int i = 0; token[i]; i++) {
			token[i] = towupper(token[i]);
		}

		// ���Բ���Ԥ�������
		WORD vk = GetKeyVk(token);
		if (vk != 0) {
			vkList[(*count)++] = vk;
		}
		else {
			// �������ַ���
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


// ģ�ⰴ��
void simulateKeys(WORD* vkList, int count) {
	INPUT inputs[MAX_KEYS * 2] = { 0 };
	int inputCount = 0;

	// �������а���
	for (int i = 0; i < count; i++) {
		inputs[inputCount].type = INPUT_KEYBOARD;
		inputs[inputCount].ki.wVk = vkList[i];
		inputCount++;
	}

	// �ͷ����а�����˳���෴��
	for (int i = count - 1; i >= 0; i--) {
		inputs[inputCount].type = INPUT_KEYBOARD;
		inputs[inputCount].ki.wVk = vkList[i];
		inputs[inputCount].ki.dwFlags = KEYEVENTF_KEYUP;
		inputCount++;
	}

	// ��������
	SendInput(inputCount, inputs, sizeof(INPUT));
}

// �������뷨�л�
void sendInputHotkey() {
	wchar_t shortcut[256] = { 0 };// ���Ⱥ�¼��ʱһ��
	WORD vkList[MAX_KEYS];
	int keyCount = 0;

	// ��ȡINI�ļ�
	wchar_t iniPath[MAX_PATH] = { 0 };
	GetAppIni(iniPath, MAX_PATH);
	GetPrivateProfileStringW(SECTION_NAME, L"Input_hotkey", L"", shortcut, 256, iniPath);

	// û���ÿ�ݼ�
	if (shortcut[0] == L'\0') {
		return;
	}

	// ������ݼ�
	if (!parseShortcut(shortcut, vkList, &keyCount)) {
		return;
	}

	// ģ�ⰴ��
	simulateKeys(vkList, keyCount);
}

// �ͼ����̹��Ӵ�����
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	static bool modifierStates[256] = { 0 }; // ����ÿ��������״̬

	if (nCode >= 0) {
		KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
		WPARAM vkCode = kb->vkCode;

		// ��鵱ǰ�����Ƿ���Ŀ�� Edit Control ��
		// ��ȡ��ǰ�н���Ĵ���
		HWND focusedWnd = GetForegroundWindow();
		if (focusedWnd) {
			// ��ȡ���㴰���е��ӿؼ�������У�
			HWND focusedCtrl = GetFocus();
			// �жϽ����Ƿ���Ŀ�� Edit �ؼ���g_hEdit��
			if (focusedCtrl != g_hEditInputHotkey) {
				// ���㲻��Ŀ�� Edit��������ֱ�ӽ���ϵͳ
				return CallNextHookEx(g_hookInput, nCode, wParam, lParam);
			}
		}
		else {
			// �޽��㴰�ڣ�������
			return CallNextHookEx(g_hookInput, nCode, wParam, lParam);
		}

		// ���°���״̬
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			modifierStates[vkCode] = true;
		}
		else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
			modifierStates[vkCode] = false;
		}

		// ֻ�����������¼�
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			// ���֮ǰ�Ŀ�ݼ��ַ���
			memset(g_hotkeyString, 0, sizeof(g_hotkeyString));

			// ������п��ܵ����μ�״̬�������ظ�����
			bool ctrl = modifierStates[VK_LCONTROL] || modifierStates[VK_RCONTROL];
			bool shift = modifierStates[VK_LSHIFT] || modifierStates[VK_RSHIFT];
			bool alt = modifierStates[VK_LMENU] || modifierStates[VK_RMENU];
			bool win = modifierStates[VK_LWIN] || modifierStates[VK_RWIN];

			// �������μ����֣��� Ctrl+Shift+Alt+Win ˳��
			if (ctrl) wcscat_s(g_hotkeyString, _countof(g_hotkeyString), L"Ctrl+");
			if (shift) wcscat_s(g_hotkeyString, _countof(g_hotkeyString), L"Shift+");
			if (alt) wcscat_s(g_hotkeyString, _countof(g_hotkeyString), L"Alt+");
			if (win) wcscat_s(g_hotkeyString, _countof(g_hotkeyString), L"Win+");

			// ��������μ��������£���û���������μ�����ֻ��ʾ�����μ�
			if (IsModifierKey(vkCode)) {
				// ����Ƿ�������ͬ���͵����μ�������
				bool hasOtherCtrl = ctrl && (vkCode != VK_LCONTROL && vkCode != VK_RCONTROL);
				bool hasOtherShift = shift && (vkCode != VK_LSHIFT && vkCode != VK_RSHIFT);
				bool hasOtherAlt = alt && (vkCode != VK_LMENU && vkCode != VK_RMENU);
				bool hasOtherWin = win && (vkCode != VK_LWIN && vkCode != VK_RWIN);

				// ���û������ͬ���͵����μ�����ֻ��ʾ�����μ�
				if (!hasOtherCtrl && !hasOtherShift && !hasOtherAlt && !hasOtherWin) {
					const wchar_t* keyName = GetKeyName(vkCode);
					if (keyName) {
						wcscpy_s(g_hotkeyString, _countof(g_hotkeyString), keyName);
					}
				}
			}
			// �����ȡ��������
			else {
				const wchar_t* keyName = GetKeyName(vkCode);
				wchar_t keyNameBuffer[128] = { 0 };

				if (keyName) {
					// ʹ��Ԥ����İ�������
					wcscpy_s(keyNameBuffer, _countof(keyNameBuffer), keyName);
				}
				else {
					// ������ĸ������ʾ��д��ʽ
					if (vkCode >= 'A' && vkCode <= 'Z') {
						keyNameBuffer[0] = (wchar_t)vkCode;
					}
					// �������ּ���ֱ����ʾ����
					else if (vkCode >= '0' && vkCode <= '9') {
						keyNameBuffer[0] = (wchar_t)vkCode;
					}
					// ������ʹ��ϵͳ������ȡ����
					else {
						UINT scanCode = kb->scanCode;
						GetKeyNameTextW(scanCode << 16, keyNameBuffer, _countof(keyNameBuffer));

						// �����ȡʧ�ܣ�ʹ����������ʮ�����Ʊ�ʾ
						if (wcslen(keyNameBuffer) == 0) {
							swprintf_s(keyNameBuffer, _countof(keyNameBuffer), L"0x%02X", (UINT)vkCode);
						}
					}
				}

				// ��Ӱ�������
				wcscat_s(g_hotkeyString, _countof(g_hotkeyString), keyNameBuffer);
			}

			// ������һ���ַ��ǼӺţ����Ƴ���
			size_t len = wcslen(g_hotkeyString);
			if (len > 0 && g_hotkeyString[len - 1] == L'+') {
				g_hotkeyString[len - 1] = L'\0';
			}

			// ���±༭���ı�
			if (g_hEditInputHotkey) SendMessageW(g_hEditInputHotkey, WM_SETTEXT, 0, (LPARAM)g_hotkeyString);

			// �������а����¼�
			return 1;
		}
	}

	// ������Ϣ����ϵͳ����
	return CallNextHookEx(g_hookInput, nCode, wParam, lParam);
}

// ���ù���
INT_PTR CALLBACK SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool hotkeyDisabled = false;
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		// �ڶԻ�������ʱж�ع���
	case WM_DESTROY:
		if (g_hookInput) {
			UnhookWindowsHookEx(g_hookInput);
			g_hookInput = NULL;
		}
		break;
	case WM_INITDIALOG: {
		// ��װ���̹���
		g_hookInput = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleW(NULL), 0);
		/*if (!g_hookInput) {
			MessageBoxW(hDlg, L"�޷���װ���̹���!", L"����", MB_ICONERROR);
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

			//ʹ�����·����GetOpenFileNameӰ�죬����Ҫ��ȫ·��
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

// �����ť�󵯳��ļ�ѡ��Ի���
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

	//GetOpenFileName �Ի���Ĭ�ϻὫ��ǰ����Ŀ¼��CWD������Ϊ�û����ѡ���Ŀ¼
	//���Ժ���ʹ�����·������ .\EasyPuTTY.ini��ʱ���ļ��ᱻд�뵽��Ԥ��λ�á�
	//����дini�ļ�Ҫʹ��ȫ·��
	if (GetOpenFileName(&ofn)) {
		// ��ȡѡ����ļ�·�������õ��༭�ؼ���
		HWND hEdit = GetDlgItem(hwnd, nIDDlgItem);
		SetWindowText(hEdit, szFile);
	}
}

void showError(HWND hwnd, const wchar_t* showMsg)
{
	// ��ȡ�������
	DWORD errorCode = GetLastError();
	wchar_t errorMsg[256] = { 0 };

	// ��ʽ��������Ϣ
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		errorMsg,
		256,
		NULL
	);

	// ��ʾ������Ϣ
	wchar_t fullError[512];
	swprintf_s(fullError, 512, GetString(IDS_ERROR_FORMAT), showMsg, errorCode, errorMsg);
	MessageBox(hwnd, fullError, GetString(IDS_MESSAGE_CAPTION), MB_ICONERROR);
}

// ��֤ƾ֤
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
			// ����·��
			GetPuttyCredentialPath(dirPath, MAX_PATH);
			CreateDirectoryIfNotExists(dirPath);
			PathCombine(iniPath, dirPath, name);  // �ϲ�Ŀ¼���ļ���
			swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//ǿ�����Ӻ�׺

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
				// �����ڿ����� �������ñ���ˢ������
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
		else if (LOWORD(wParam) == IDC_CLEAR) {//�������
			HWND hEdit;
			hEdit = GetDlgItem(hDlg, IDC_NAME);
			SetWindowText(hEdit, L"");
			return (INT_PTR) FALSE;
		}
		else if (LOWORD(wParam) == IDC_RESET) {//����
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
		else if (LOWORD(wParam) == IDC_DEL) {// ɾ��ѡ����
			wchar_t dirPath[MAX_PATH] = { 0 };
			wchar_t iniPath[MAX_PATH] = { 0 };

			HWND hListBox = GetDlgItem(hDlg, IDC_LIST_NAME);
			int selIndex = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
			if (selIndex != LB_ERR)
			{
				wchar_t itemText[256];
				SendMessage(hListBox, LB_GETTEXT, selIndex, (LPARAM)itemText);
				GetPuttyCredentialPath(dirPath, MAX_PATH);
				PathCombine(iniPath, dirPath, itemText);  // �ϲ�Ŀ¼���ļ���
				swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//ǿ�����Ӻ�׺
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
		else if (LOWORD(wParam) == IDC_EDIT) {//�༭
			wchar_t dirPath[MAX_PATH] = { 0 };
			wchar_t iniPath[MAX_PATH] = { 0 };
			HWND hListBox = GetDlgItem(hDlg, IDC_LIST_NAME);
			// �����б���ѡ��仯
			int selIndex = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
			if (selIndex != LB_ERR)
			{
				wchar_t itemText[256];
				CredentialInfo config;
				HWND hEdit;
				SendMessage(hListBox, LB_GETTEXT, selIndex, (LPARAM)itemText);
				// ����·��
				GetPuttyCredentialPath(dirPath, MAX_PATH);
				PathCombine(iniPath, dirPath, itemText);  // �ϲ�Ŀ¼���ļ���
				swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//ǿ�����Ӻ�׺
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


// ���Ӧ��
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

		// ���ҵ�ǰѡ��ĻỰ�м��
		BOOL ret = FindSelectedSession(findName, MAX_PATH);
		if (ret) {
			// ����·��
			GetProgramPath(dirPath, MAX_PATH);
			PathCombine(iniPath, dirPath, findName);  // �ϲ�Ŀ¼���ļ���
			swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//ǿ�����Ӻ�׺
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
			// ����·��
			GetProgramPath(dirPath, MAX_PATH);
			CreateDirectoryIfNotExists(dirPath);
			PathCombine(iniPath, dirPath, name);  // �ϲ�Ŀ¼���ļ���
			swprintf(iniPath, MAX_PATH, L"%s.ini", iniPath);//ǿ�����Ӻ�׺

			BOOL result = WritePrivateProfileString(SECTION_NAME, L"Name", name, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Path", path, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Params", params, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Tags", tags, iniPath);

			if (!result) {
				showError(hDlg, GetString(IDS_ADD_FAIL));
				return FALSE;
			}

			// ��ǰ��ǩ
			HWND tabCtrlWinHandle = g_tabWindowsInfo->tabCtrlWinHandle;
			int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
			if (sel != -1) {
				TCCUSTOMITEM tabCtrlItemInfo = { 0 };
				getTabItemInfo(tabCtrlWinHandle, sel, &tabCtrlItemInfo);
				// ��overview��ǩ��
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


// ���������������Ϣ�������
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
