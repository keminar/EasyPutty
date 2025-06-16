#pragma once

#include "apputils.h"
#include "enum.h"
#include "overview.h"
#include "EasyPuTTY.h"

static HINSTANCE g_appInstance;
static TabWindowsInfo* g_tabWindowsInfo = NULL;

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

void GetPuttySessionsPath(wchar_t* buffer, size_t bufferSize) {
	GetCurrentDirectoryPath(buffer, bufferSize);
	// ׷��Ŀ��·��
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
	if (!show) {
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE; // ���ش���
	}

	// ������Ϣ
	PROCESS_INFORMATION pi = { 0 };

	// �����½���
	if (CreateProcess(
		appPath,            // ����·��
		NULL,               // �����в���
		NULL,               // ���̰�ȫ����
		NULL,               // �̰߳�ȫ����
		FALSE,              // ���̳о��
		CREATE_NO_WINDOW,   // �ؼ���־�������޴��ڵĽ���
		NULL,               // ��������
		NULL,               // ��ǰĿ¼
		&si,                // ������Ϣ
		&pi                 // ������Ϣ
	)) {

		// �رս��̺��߳̾��
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		if (!show) {
			MessageBoxW(NULL, L"�����ɹ�", L"��ʾ", MB_OK);
		}
		return 0;
	}
	else {
		MessageBoxW(NULL, L"����ʧ��", L"��ʾ", MB_OK);
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
	case WM_INITDIALOG:
		createEnum(g_appInstance, g_tabWindowsInfo, hDlg);
		return (INT_PTR)TRUE;

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

// ���ҵ�ǰѡ��ĻỰ��
BOOL FindSelectedSession(wchar_t* name, int nameLen) {
	// ��ǰ��ǩ
	HWND tabCtrlWinHandle = (g_tabWindowsInfo)->tabCtrlWinHandle;
	int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
	if (sel != -1) {
		TCCUSTOMITEM tabCtrlItemInfo = getTabItemInfo(tabCtrlWinHandle, sel);
		// ��overview��ǩ��
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
			PathAddExtension(iniPath, L".ini");   // ��� .ini ��չ
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
				MessageBoxW(hDlg, L"���Ϊ����", L"����", MB_OK);
				return FALSE;
			}
			if (hostname[0] == L'\0') {
				MessageBoxW(hDlg, L"��ַΪ����", L"����", MB_OK);
				return FALSE;
			}
			// ����·��
			GetPuttySessionsPath(dirPath, MAX_PATH);
			CreateDirectoryIfNotExists(dirPath);
			PathCombine(iniPath, dirPath, name);  // �ϲ�Ŀ¼���ļ���
			PathAddExtension(iniPath, L".ini");   // ��� .ini ��չ

			BOOL result = WritePrivateProfileString(SECTION_NAME, L"Name", name, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"HostName", hostname, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Port", port, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"ConnectType", connectType, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Credential", credential, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Tags", tags, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"OtherParams", otherParams, iniPath);
			if (!result) {
				showError(hDlg, L"���ʧ��");
				return FALSE;
			}

			// ��ǰ��ǩ
			HWND tabCtrlWinHandle = g_tabWindowsInfo->tabCtrlWinHandle;
			int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
			if (sel != -1) {
				TCCUSTOMITEM tabCtrlItemInfo = getTabItemInfo(tabCtrlWinHandle, sel);
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


// ���ù���
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

			//ʹ�����·����GetOpenFileNameӰ�죬����Ҫ��ȫ·��
			GetAppIni(iniPath, MAX_PATH);

			BOOL result = WritePrivateProfileString(SECTION_NAME, L"Putty", putty, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Winscp", winscp, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Filezilla", filezilla, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Putty_params", params, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Puttygen", puttygen, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Pageant", pageant, iniPath);
			if (!result) {
				showError(hDlg, L"���ʧ��");
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
			setBrowser(hDlg, L"��ִ���ļ�\0*.exe\0�����ļ�\0*.*\0", IDC_PUTTY);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_WINSCP) {
			setBrowser(hDlg, L"��ִ���ļ�\0*.exe\0�����ļ�\0*.*\0", IDC_WINSCP);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_FILEZILLA) {
			setBrowser(hDlg, L"��ִ���ļ�\0*.exe\0�����ļ�\0*.*\0", IDC_FILEZILLA);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_PUTTYGEN) {
			setBrowser(hDlg, L"��ִ���ļ�\0*.exe\0�����ļ�\0*.*\0", IDC_PUTTYGEN);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_BROWSER_PAGEANT) {
			setBrowser(hDlg, L"��ִ���ļ�\0*.exe\0�����ļ�\0*.*\0", IDC_PAGEANT);
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
	swprintf_s(fullError, 512, L"%s���������: %lu\n%ls", showMsg, errorCode, errorMsg);
	MessageBox(hwnd, fullError, L"����", MB_ICONERROR);
}
// ��֤ƾ֤
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
				MessageBoxW(hDlg, L"����Ϊ����", L"����", MB_OK);
				return FALSE;
			}
			// ����·��
			GetPuttyCredentialPath(dirPath, MAX_PATH);
			CreateDirectoryIfNotExists(dirPath);
			PathCombine(iniPath, dirPath, name);  // �ϲ�Ŀ¼���ļ���
			PathAddExtension(iniPath, L".ini");   // ��� .ini ��չ

			BOOL result = WritePrivateProfileString(SECTION_NAME, L"Name", name, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"UserName", username, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"Password", password, iniPath);
			WritePrivateProfileString(SECTION_NAME, L"PrivateKey", ppkfile, iniPath);

			if (!result) {
				showError(hDlg, L"���ʧ��");
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
			setBrowser(hDlg, L"PuTTY˽Կ.ppk\0*.ppk\0�����ļ�\0*.*\0", IDC_PPK);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
