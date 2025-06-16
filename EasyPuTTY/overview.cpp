#pragma once

#include "overview.h"

static HINSTANCE g_appInstance;

// �������ڵ����໯����
LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// ��ȡԭʼ���ڹ���
	WNDPROC originalProc = (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	switch (uMsg) {
	case WM_GETMAINWINDOW:
		// ���ϼ����ڲ�ѯ�����ھ��
		return (LRESULT)GetParent(hwnd);
	case WM_SIZE: {
		HWND hListView = GetDlgItem(hwnd, ID_LIST_VIEW);
		// �����б���ͼ��С����Ӧ��������
		if (hListView) {
			RECT rc;
			GetClientRect(hwnd, &rc);
			MoveWindow(hListView, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
		}
		break;
	}
	case WM_NOTIFY: {
		// ���� ListView ֪ͨ��Ϣ
		LPNMHDR pnmh = (LPNMHDR)lParam;
		HWND hListView = GetDlgItem(hwnd, ID_LIST_VIEW);
		if (pnmh->hwndFrom == hListView) {
			if (pnmh->code == NM_DBLCLK) {
				// ˫���¼�����
				LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)lParam;
				if (pnmia->iItem != -1) {
					execCommand(hwnd, hListView, pnmia->iItem);
				}
			}
			else if (pnmh->code == LVN_KEYDOWN) {
				LPNMLVKEYDOWN pnmlvkd = (LPNMLVKEYDOWN)lParam;
				if (pnmlvkd->wVKey == VK_RETURN) {  // �س���
					int selectedItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
					if (selectedItem != -1) {
						execCommand(hwnd, hListView, selectedItem);
					}
				}
			}
			else if (pnmh->code == NM_RCLICK) {
				// ��ȡ���λ�ã���Ļ���꣩
				POINT pt;
				GetCursorPos(&pt);

				// ����Ƿ��Ҽ������ĳ����Ŀ
				LVHITTESTINFO hitTestInfo;
				hitTestInfo.pt = pt;
				ScreenToClient(hListView, &hitTestInfo.pt);

				// ���Ե��λ��
				int itemIndex = ListView_HitTest(hListView, &hitTestInfo);

				// ��������ĳ����Ŀ��ѡ����
				if (itemIndex != -1) {
					ListView_SetItemState(hListView, itemIndex, LVIS_SELECTED, LVIS_SELECTED);
				}
				// ���ز˵���Դ
				HMENU hMenu = LoadMenu(g_appInstance, MAKEINTRESOURCE(IDR_SESSION));
				HMENU hSubMenu = GetSubMenu(hMenu, 0);

				// ��ʾ�Ҽ��˵���TrackPopupMenu��������������ȴ��û�ѡ��˵��
				TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
					pt.x, pt.y, 0, hwnd, NULL);

				// �ͷŲ˵���Դ
				DestroyMenu(hMenu);
			}
		}
	}
					break;
	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
		case ID_RUN_COMMAND: {
			HWND hListView = GetDlgItem(hwnd, ID_LIST_VIEW);
			int selectedItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
			if (selectedItem != -1) {
				execCommand(hwnd, hListView, selectedItem);
			}
			break;
		}
		}
		break;
	}
	}

	// ������Ϣ����ԭʼ���ڹ��̴���
	return CallWindowProcW(originalProc, hwnd, uMsg, wParam, lParam);
}

// ִ���������±�ǩ��
void execCommand(HWND hwnd, HWND hListView, int selectedItem) {
	wchar_t szText[256] = { 0 };
	int column = 2;//�ڼ��е�ֵ
	ListView_GetItemText(hListView, selectedItem, column, szText, sizeof(szText));
	// ͨ�������Զ�����Ϣ��ȡ�����ھ��
	HWND mainWindow = (HWND)SendMessage(hwnd, WM_GETMAINWINDOW, 0, 0);
	if (mainWindow) {
		// ת����ť�����Ϣ��������
		SendMessage(mainWindow, WM_COMMAND, ID_LIST_ATTACH, (LPARAM)&szText[0]);
	}
}

// ��������
void InitOverview(HINSTANCE hInstance, struct TabWindowsInfo *tabWindowsInfo, HWND hostWindow) {
	wchar_t sessionsPath[MAX_PATH] = { 0 }, credentialPath[MAX_PATH] = { 0 };
	wchar_t iniPath[MAX_PATH] = { 0 }, programPath[MAX_PATH] = { 0 };
	wchar_t putty[MAX_PATH] = { 0 }, putty_params[MAX_PATH] = { 0 };
	int sessionCount = 0, credentialCount = 0, programCount = 0;
	wchar_t** sessionFileList = NULL;
	wchar_t** credentialFileList = NULL;
	wchar_t** programFileList = NULL;
	SessionInfo sessionConfig = { 0 };
	ProgramInfo programConfig = { 0 };
	CredentialInfo credentialConfig = { 0 };
	CredentialInfo* foundCredential = NULL;
	ConfigMap* credentialMap = NULL;
	wchar_t command[MAX_COMMAND_LEN] = { 0 };
	int nItem = 0;

	HWND hListView = CreateWindowW(
		WC_LISTVIEW,
		L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS | LVS_SHOWSELALWAYS,
		0, 0, 500, 300, // ��ʼλ�úʹ�С
		hostWindow,
		(HMENU)ID_LIST_VIEW, // �ؼ�ID
		hInstance,
		NULL
	);

	if (!hListView) {
		MessageBoxW(NULL, L"�޷������б���ͼ", L"����", MB_OK | MB_ICONERROR);
		return;
	}
	g_appInstance = hInstance;
	// ��������
	SendMessageW(hListView, WM_SETFONT, (WPARAM)(tabWindowsInfo->editorFontHandle), 0);

	// �����б���ͼ��չ��ʽ
	ListView_SetExtendedListViewStyle(hListView,
		LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// ��ʼ���б���ͼ��
	InitializeListViewColumns(hListView);

	// putty·��
	GetAppIni(iniPath, MAX_PATH);
	GetPrivateProfileStringW(SECTION_NAME, L"Putty", L"", putty, MAX_PATH, iniPath);
	if (putty[0] == L'\0') {
		wcscpy_s(putty, MAX_PATH, L".\\putty.exe");
	}
	GetPrivateProfileStringW(SECTION_NAME, L"Putty_params", L"", putty_params, MAX_PATH, iniPath);
	if (putty_params[0] != L'\0') {
		swprintf(putty, MAX_COMMAND_LEN, L"%s %s", putty, putty_params);
	}

	// �Զ������
	GetProgramPath(programPath, MAX_PATH);
	programFileList = ListIniFiles(programPath, &programCount);
	for (int i = 0; i < programCount; i++) {
		if (programFileList[i] != NULL) {
			ReadProgramFromIni(programFileList[i], &programConfig);
			if (programConfig.command[0] != L'\0') {
				AddListViewItem(hListView, nItem, programConfig.name, L"�Զ���", programConfig.command, programConfig.tags, L"", L"��");
				nItem++;
			}
		}
	}

	// ƾ֤
	GetPuttyCredentialPath(credentialPath, MAX_PATH);
	credentialFileList = ListIniFiles(credentialPath, &credentialCount);
	credentialMap = initConfigMap(credentialCount);
	if (credentialMap) {
		for (int i = 0; i < credentialCount; i++) {
			if (credentialFileList[i] != NULL) {
				ReadCredentialFromIni(credentialFileList[i], &credentialConfig);
				addConfig(credentialMap, credentialConfig.name, credentialConfig.userName, credentialConfig.password, credentialConfig.privateKey);
			}
		}
	}
	// �Ự
	GetPuttySessionsPath(sessionsPath, MAX_PATH);
	sessionFileList = ListIniFiles(sessionsPath, &sessionCount);
	for (int i = 0; i < sessionCount; i++) {
		if (sessionFileList[i] != NULL) {
			// ��ȡ����
			ReadSessionFromIni(sessionFileList[i], &sessionConfig);
			if (sessionConfig.hostName[0] == L'\0') {
				continue;
			}
			foundCredential = findConfigByName(credentialMap, sessionConfig.credential);
			if (foundCredential) {
				if (foundCredential->userName[0] != L'\0') {
					swprintf(command, MAX_COMMAND_LEN, L"%s -ssh %s@%s", putty, foundCredential->userName, sessionConfig.hostName);
				}
				else {
					swprintf(command, MAX_COMMAND_LEN, L"%s -ssh %s", putty, sessionConfig.hostName);
				}
				if (foundCredential->privateKey[0] != L'\0') {
					swprintf(command, MAX_COMMAND_LEN, L"%s -i %s", command, foundCredential->privateKey);
				}
				if (foundCredential->password[0] != L'\0') {
					swprintf(command, MAX_COMMAND_LEN, L"%s -pw %s", command, foundCredential->password);
				}
			}
			else {
				swprintf(command, MAX_COMMAND_LEN, L"%s -ssh %s", putty, sessionConfig.hostName);
			}
			if (wcscmp(sessionConfig.connectType, L"SSHv1") == 0) {
				swprintf(command, MAX_COMMAND_LEN, L"%s -1", command);
			}
			if (sessionConfig.port != 22) {
				swprintf(command, MAX_COMMAND_LEN, L"%s -P %d", command, sessionConfig.port);
			}
			if (sessionConfig.otherParams[0] != L'\0') {
				swprintf(command, MAX_COMMAND_LEN, L"%s %s", command, sessionConfig.otherParams);
			}
			AddListViewItem(hListView, nItem, sessionConfig.name, L"PuTTY", command, sessionConfig.tags, sessionConfig.credential, L"��");
			nItem++;
		}
	}
	//�ͷ��ڴ�
	FreeConfigMap(credentialMap);
	FreeFileList(credentialFileList, credentialCount);
	FreeFileList(sessionFileList, sessionCount);

	// ���໯��������
	WNDPROC originalProc = (WNDPROC)SetWindowLongPtrW(hostWindow, GWLP_WNDPROC, (LONG_PTR)HostWindowProc);
	// �洢ԭʼ���ڹ��̣����ں�������
	SetWindowLongPtrW(hostWindow, GWLP_USERDATA, (LONG_PTR)originalProc);
}

// ��ʼ���б���ͼ��
void InitializeListViewColumns(HWND hWndListView) {
	LVCOLUMNW lvc = { 0 };
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;

	lvc.iSubItem = 0;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"���";
	ListView_InsertColumn(hWndListView, lvc.iSubItem, &lvc);

	lvc.iSubItem = 1;
	lvc.cx = 110;
	lvc.pszText = (LPWSTR)L"����";
	ListView_InsertColumn(hWndListView, lvc.iSubItem, &lvc);

	lvc.iSubItem = 2;
	lvc.cx = 500;
	lvc.pszText = (LPWSTR)L"����";
	ListView_InsertColumn(hWndListView, lvc.iSubItem, &lvc);

	lvc.iSubItem = 3;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"��ǩ";
	ListView_InsertColumn(hWndListView, lvc.iSubItem, &lvc);

	lvc.iSubItem = 4;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"ƾ֤";
	ListView_InsertColumn(hWndListView, lvc.iSubItem, &lvc);

	lvc.iSubItem = 5;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"�л����뷨";
	ListView_InsertColumn(hWndListView, lvc.iSubItem, &lvc);
}

// ����б���
void AddListViewItem(HWND hWndListView, int nItem, const wchar_t* name, const wchar_t* type, const wchar_t* command, const wchar_t* tags, const wchar_t* credential, const wchar_t* input) {
	LVITEMW lvi = { 0 };
	lvi.mask = LVIF_TEXT;
	lvi.iItem = nItem;
	lvi.iSubItem = 0;
	lvi.pszText = (LPWSTR)name;
	ListView_InsertItem(hWndListView, &lvi);
	lvi.iSubItem++;

	ListView_SetItemText(hWndListView, lvi.iItem, lvi.iSubItem, (LPWSTR)type);
	lvi.iSubItem++;
	ListView_SetItemText(hWndListView, lvi.iItem, lvi.iSubItem, (LPWSTR)command);
	lvi.iSubItem++;
	ListView_SetItemText(hWndListView, lvi.iItem, lvi.iSubItem, (LPWSTR)tags);
	lvi.iSubItem++;
	ListView_SetItemText(hWndListView, lvi.iItem, lvi.iSubItem, (LPWSTR)credential);
	lvi.iSubItem++;
	ListView_SetItemText(hWndListView, lvi.iItem, lvi.iSubItem, (LPWSTR)input);
}

// ��ȡָ��Ŀ¼�µ�����INI�ļ�
wchar_t** ListIniFiles(const wchar_t* directoryPath, int* fileCount) {
	WIN32_FIND_DATAW findData;
	HANDLE hFind;
	wchar_t searchPath[MAX_PATH] = { 0 };
	wchar_t** fileList = NULL;
	int count = 0;
	int capacity = 10;  // ��ʼ����

	// ��������·��
	wcscpy_s(searchPath, MAX_PATH, directoryPath);
	wcscat_s(searchPath, MAX_PATH, L"\\*.ini");

	// ��ʼ���ҵ�һ��ƥ����ļ�
	hFind = FindFirstFileW(searchPath, &findData);
	if (hFind == INVALID_HANDLE_VALUE) {
		*fileCount = 0;
		return NULL;
	}

	// �����ʼ�ڴ�
	fileList = (wchar_t**)malloc(capacity * sizeof(wchar_t*));
	if (fileList == NULL) {
		FindClose(hFind);
		*fileCount = 0;
		return NULL;
	}

	// ö������ƥ����ļ�
	do {
		// ���� . �� .. Ŀ¼
		if (wcscmp(findData.cFileName, L".") == 0 ||
			wcscmp(findData.cFileName, L"..") == 0) {
			continue;
		}

		// ֻ�����ļ���������Ŀ¼
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			// ��������·������
			size_t pathLen = wcslen(directoryPath) + wcslen(findData.cFileName) + 2;

			// �����ڴ沢��������·��
			fileList[count] = (wchar_t*)malloc(pathLen * sizeof(wchar_t));
			if (fileList[count] == NULL) {
				// �ڴ����ʧ�ܣ������ѷ�����ڴ�
				for (int i = 0; i < count; i++) {
					free(fileList[i]);
				}
				free(fileList);
				FindClose(hFind);
				*fileCount = 0;
				return NULL;
			}

			wcscpy_s(fileList[count], pathLen, directoryPath);
			wcscat_s(fileList[count], pathLen, L"\\");
			wcscat_s(fileList[count], pathLen, findData.cFileName);

			count++;

			// ���������������չ����
			if (count >= capacity) {
				capacity *= 2;
				wchar_t** newList = (wchar_t**)realloc(fileList, capacity * sizeof(wchar_t*));
				if (newList == NULL) {
					// �ڴ����·���ʧ�ܣ������ѷ�����ڴ�
					for (int i = 0; i < count; i++) {
						free(fileList[i]);
					}
					free(fileList);
					FindClose(hFind);
					*fileCount = 0;
					return NULL;
				}
				fileList = newList;
			}
		}
	} while (FindNextFileW(hFind, &findData));

	// �رղ��Ҿ��
	FindClose(hFind);

	// �����ļ��������ļ��б�
	*fileCount = count;
	return fileList;
}

// �ͷ��ļ��б��ڴ�
void FreeFileList(wchar_t** fileList, int fileCount) {
	if (fileList != NULL) {
		for (int i = 0; i < fileCount; i++) {
			if (fileList[i] != NULL) {
				free(fileList[i]);
			}
		}
		free(fileList);
	}
}

// ��INI�ļ���ȡ������Ϣ
void ReadProgramFromIni(const wchar_t* filepath, ProgramInfo* config) {
	// ��ֹ��ζ�ȡ������Ⱦ
	ZeroMemory(config, sizeof(ProgramInfo));
	// ��ȡName
	GetPrivateProfileStringW(SECTION_NAME, L"Name", L"",
		config->name, sizeof(config->name) / sizeof(wchar_t),
		filepath);

	// ��ȡCommand
	GetPrivateProfileStringW(SECTION_NAME, L"Command", L"",
		config->command, sizeof(config->command) / sizeof(wchar_t),
		filepath);

	// ��ȡTags
	GetPrivateProfileStringW(SECTION_NAME, L"Tags", L"",
		config->tags, sizeof(config->tags) / sizeof(wchar_t),
		filepath);

}

// ��INI�ļ���ȡ������Ϣ
void ReadSessionFromIni(const wchar_t* filepath, SessionInfo* config) {
	// ��ֹ��ζ�ȡ������Ⱦ
	ZeroMemory(config, sizeof(SessionInfo));
	// ��ȡName
	GetPrivateProfileStringW(SECTION_NAME, L"Name", L"",
		config->name, sizeof(config->name) / sizeof(wchar_t),
		filepath);

	// ��ȡHostName
	GetPrivateProfileStringW(SECTION_NAME, L"HostName", L"",
		config->hostName, sizeof(config->hostName) / sizeof(wchar_t),
		filepath);

	// ��ȡPort
	config->port = GetPrivateProfileIntW(SECTION_NAME, L"Port", 22, filepath);

	// ��ȡConnectType
	GetPrivateProfileStringW(SECTION_NAME, L"ConnectType", L"",
		config->connectType, sizeof(config->connectType) / sizeof(wchar_t),
		filepath);

	// ��ȡCredential
	GetPrivateProfileStringW(SECTION_NAME, L"Credential", L"",
		config->credential, sizeof(config->credential) / sizeof(wchar_t),
		filepath);

	// ��ȡTags
	GetPrivateProfileStringW(SECTION_NAME, L"Tags", L"",
		config->tags, sizeof(config->tags) / sizeof(wchar_t),
		filepath);
	// �Զ������
	GetPrivateProfileStringW(SECTION_NAME, L"OtherParams", L"",
		config->otherParams, sizeof(config->otherParams) / sizeof(wchar_t),
		filepath);

}

// ��INI�ļ���ȡ������Ϣ
void ReadCredentialFromIni(const wchar_t* filepath, CredentialInfo* config) {
	// ��ֹ��ζ�ȡ������Ⱦ
	ZeroMemory(config, sizeof(CredentialInfo));
	// ��ȡName
	GetPrivateProfileStringW(SECTION_NAME, L"Name", L"",
		config->name, sizeof(config->name) / sizeof(wchar_t),
		filepath);

	GetPrivateProfileStringW(SECTION_NAME, L"UserName", L"",
		config->userName, sizeof(config->userName) / sizeof(wchar_t),
		filepath);

	GetPrivateProfileStringW(SECTION_NAME, L"Password", L"",
		config->password, sizeof(config->password) / sizeof(wchar_t),
		filepath);

	GetPrivateProfileStringW(SECTION_NAME, L"PrivateKey", L"",
		config->privateKey, sizeof(config->privateKey) / sizeof(wchar_t),
		filepath);
}

// �����ַ����Ĺ�ϣֵ
unsigned int hash(const wchar_t* str, int capacity) {
	unsigned int hashval = 0;
	while (*str) {
		hashval = (*str++) + (hashval << 6) + (hashval << 16) - hashval;
	}
	return hashval % capacity;
}


// ��ʼ������ӳ���
ConfigMap* initConfigMap(int capacity) {
	ConfigMap* map = (ConfigMap*)malloc(sizeof(ConfigMap));
	if (map == NULL) {
		return NULL;
	}

	map->capacity = capacity;
	map->table = (HashNode**)calloc(map->capacity, sizeof(HashNode*));
	if (map->table == NULL) {
		free(map);
		return NULL;
	}
	map->count = 0;
	return map;
}

// ���������
int addConfig(ConfigMap* map, const wchar_t* name, const wchar_t* username,
	const wchar_t* password, const wchar_t* privateKey) {
	if (map == NULL || name == NULL) return -1;

	// �����ϣֵ
	unsigned int index = hash(name, map->capacity);

	// ����Ƿ��Ѵ�����ͬ�ļ�
	HashNode* current = map->table[index];
	while (current != NULL) {
		if (wcscmp(current->key, name) == 0) {
			// �����Ѵ��ڵ���
			wcscpy_s(current->value.userName, sizeof(current->value.userName) / sizeof(wchar_t), username);
			wcscpy_s(current->value.password, sizeof(current->value.password) / sizeof(wchar_t), password);
			wcscpy_s(current->value.privateKey, sizeof(current->value.privateKey) / sizeof(wchar_t), privateKey);
			return 0;
		}
		// ��������ͬ�����Ƿ���ͬ��������
		current = current->next;
	}

	// �����½ڵ�
	HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
	if (newNode == NULL) return -1;

	wcscpy_s(newNode->key, sizeof(newNode->key) / sizeof(wchar_t), name);
	wcscpy_s(newNode->value.name, sizeof(newNode->value.name) / sizeof(wchar_t), name);
	wcscpy_s(newNode->value.userName, sizeof(newNode->value.userName) / sizeof(wchar_t), username);
	wcscpy_s(newNode->value.password, sizeof(newNode->value.password) / sizeof(wchar_t), password);
	wcscpy_s(newNode->value.privateKey, sizeof(newNode->value.privateKey) / sizeof(wchar_t), privateKey);

	// ��ӵ�����ͷ��
	newNode->next = map->table[index];
	map->table[index] = newNode;
	map->count++;

	return 0;
}

// ����Name��������
CredentialInfo* findConfigByName(ConfigMap* map, const wchar_t* name) {
	if (map == NULL || name == NULL) return NULL;

	// �����ϣֵ
	unsigned int index = hash(name, map->capacity);

	// �������в���
	HashNode* current = map->table[index];
	while (current != NULL) {
		if (wcscmp(current->key, name) == 0) {
			return &(current->value);
		}
		current = current->next;
	}

	return NULL;
}

// �ͷ�����ӳ����ڴ�
void FreeConfigMap(ConfigMap* map) {
	if (map == NULL) return;

	for (int i = 0; i < map->capacity; i++) {
		HashNode* current = map->table[i];
		while (current != NULL) {
			HashNode* temp = current;
			current = current->next;
			free(temp);
		}
		map->table[i] = NULL;
	}
	// �ͷ�Ͱ����
	free(map->table);
	// �ͷŹ�ϣ��ṹ��
	free(map);
}
