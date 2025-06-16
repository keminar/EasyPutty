#pragma once

#include "overview.h"

static HINSTANCE g_appInstance;

// 宿主窗口的子类化过程
LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// 获取原始窗口过程
	WNDPROC originalProc = (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	switch (uMsg) {
	case WM_GETMAINWINDOW:
		// 向上级窗口查询主窗口句柄
		return (LRESULT)GetParent(hwnd);
	case WM_SIZE: {
		HWND hListView = GetDlgItem(hwnd, ID_LIST_VIEW);
		// 调整列表视图大小以适应宿主窗口
		if (hListView) {
			RECT rc;
			GetClientRect(hwnd, &rc);
			MoveWindow(hListView, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
		}
		break;
	}
	case WM_NOTIFY: {
		// 处理 ListView 通知消息
		LPNMHDR pnmh = (LPNMHDR)lParam;
		HWND hListView = GetDlgItem(hwnd, ID_LIST_VIEW);
		if (pnmh->hwndFrom == hListView) {
			if (pnmh->code == NM_DBLCLK) {
				// 双击事件处理
				LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)lParam;
				if (pnmia->iItem != -1) {
					execCommand(hwnd, hListView, pnmia->iItem);
				}
			}
			else if (pnmh->code == LVN_KEYDOWN) {
				LPNMLVKEYDOWN pnmlvkd = (LPNMLVKEYDOWN)lParam;
				if (pnmlvkd->wVKey == VK_RETURN) {  // 回车键
					int selectedItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
					if (selectedItem != -1) {
						execCommand(hwnd, hListView, selectedItem);
					}
				}
			}
			else if (pnmh->code == NM_RCLICK) {
				// 获取鼠标位置（屏幕坐标）
				POINT pt;
				GetCursorPos(&pt);

				// 检查是否右键点击了某个项目
				LVHITTESTINFO hitTestInfo;
				hitTestInfo.pt = pt;
				ScreenToClient(hListView, &hitTestInfo.pt);

				// 测试点击位置
				int itemIndex = ListView_HitTest(hListView, &hitTestInfo);

				// 如果点击了某个项目，选中它
				if (itemIndex != -1) {
					ListView_SetItemState(hListView, itemIndex, LVIS_SELECTED, LVIS_SELECTED);
				}
				// 加载菜单资源
				HMENU hMenu = LoadMenu(g_appInstance, MAKEINTRESOURCE(IDR_SESSION));
				HMENU hSubMenu = GetSubMenu(hMenu, 0);

				// 显示右键菜单（TrackPopupMenu是阻塞函数，会等待用户选择菜单项）
				TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
					pt.x, pt.y, 0, hwnd, NULL);

				// 释放菜单资源
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

	// 其他消息交给原始窗口过程处理
	return CallWindowProcW(originalProc, hwnd, uMsg, wParam, lParam);
}

// 执行命令在新标签打开
void execCommand(HWND hwnd, HWND hListView, int selectedItem) {
	wchar_t szText[256] = { 0 };
	int column = 2;//第几列的值
	ListView_GetItemText(hListView, selectedItem, column, szText, sizeof(szText));
	// 通过发送自定义消息获取主窗口句柄
	HWND mainWindow = (HWND)SendMessage(hwnd, WM_GETMAINWINDOW, 0, 0);
	if (mainWindow) {
		// 转发按钮点击消息到主窗口
		SendMessage(mainWindow, WM_COMMAND, ID_LIST_ATTACH, (LPARAM)&szText[0]);
	}
}

// 创建窗口
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
		0, 0, 500, 300, // 初始位置和大小
		hostWindow,
		(HMENU)ID_LIST_VIEW, // 控件ID
		hInstance,
		NULL
	);

	if (!hListView) {
		MessageBoxW(NULL, L"无法创建列表视图", L"错误", MB_OK | MB_ICONERROR);
		return;
	}
	g_appInstance = hInstance;
	// 设置字体
	SendMessageW(hListView, WM_SETFONT, (WPARAM)(tabWindowsInfo->editorFontHandle), 0);

	// 设置列表视图扩展样式
	ListView_SetExtendedListViewStyle(hListView,
		LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 初始化列表视图列
	InitializeListViewColumns(hListView);

	// putty路径
	GetAppIni(iniPath, MAX_PATH);
	GetPrivateProfileStringW(SECTION_NAME, L"Putty", L"", putty, MAX_PATH, iniPath);
	if (putty[0] == L'\0') {
		wcscpy_s(putty, MAX_PATH, L".\\putty.exe");
	}
	GetPrivateProfileStringW(SECTION_NAME, L"Putty_params", L"", putty_params, MAX_PATH, iniPath);
	if (putty_params[0] != L'\0') {
		swprintf(putty, MAX_COMMAND_LEN, L"%s %s", putty, putty_params);
	}

	// 自定义程序
	GetProgramPath(programPath, MAX_PATH);
	programFileList = ListIniFiles(programPath, &programCount);
	for (int i = 0; i < programCount; i++) {
		if (programFileList[i] != NULL) {
			ReadProgramFromIni(programFileList[i], &programConfig);
			if (programConfig.command[0] != L'\0') {
				AddListViewItem(hListView, nItem, programConfig.name, L"自定义", programConfig.command, programConfig.tags, L"", L"是");
				nItem++;
			}
		}
	}

	// 凭证
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
	// 会话
	GetPuttySessionsPath(sessionsPath, MAX_PATH);
	sessionFileList = ListIniFiles(sessionsPath, &sessionCount);
	for (int i = 0; i < sessionCount; i++) {
		if (sessionFileList[i] != NULL) {
			// 读取配置
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
			AddListViewItem(hListView, nItem, sessionConfig.name, L"PuTTY", command, sessionConfig.tags, sessionConfig.credential, L"是");
			nItem++;
		}
	}
	//释放内存
	FreeConfigMap(credentialMap);
	FreeFileList(credentialFileList, credentialCount);
	FreeFileList(sessionFileList, sessionCount);

	// 子类化宿主窗口
	WNDPROC originalProc = (WNDPROC)SetWindowLongPtrW(hostWindow, GWLP_WNDPROC, (LONG_PTR)HostWindowProc);
	// 存储原始窗口过程，用于后续调用
	SetWindowLongPtrW(hostWindow, GWLP_USERDATA, (LONG_PTR)originalProc);
}

// 初始化列表视图列
void InitializeListViewColumns(HWND hWndListView) {
	LVCOLUMNW lvc = { 0 };
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;

	lvc.iSubItem = 0;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"简称";
	ListView_InsertColumn(hWndListView, lvc.iSubItem, &lvc);

	lvc.iSubItem = 1;
	lvc.cx = 110;
	lvc.pszText = (LPWSTR)L"类型";
	ListView_InsertColumn(hWndListView, lvc.iSubItem, &lvc);

	lvc.iSubItem = 2;
	lvc.cx = 500;
	lvc.pszText = (LPWSTR)L"命令";
	ListView_InsertColumn(hWndListView, lvc.iSubItem, &lvc);

	lvc.iSubItem = 3;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"标签";
	ListView_InsertColumn(hWndListView, lvc.iSubItem, &lvc);

	lvc.iSubItem = 4;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"凭证";
	ListView_InsertColumn(hWndListView, lvc.iSubItem, &lvc);

	lvc.iSubItem = 5;
	lvc.cx = 200;
	lvc.pszText = (LPWSTR)L"切换输入法";
	ListView_InsertColumn(hWndListView, lvc.iSubItem, &lvc);
}

// 添加列表项
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

// 获取指定目录下的所有INI文件
wchar_t** ListIniFiles(const wchar_t* directoryPath, int* fileCount) {
	WIN32_FIND_DATAW findData;
	HANDLE hFind;
	wchar_t searchPath[MAX_PATH] = { 0 };
	wchar_t** fileList = NULL;
	int count = 0;
	int capacity = 10;  // 初始容量

	// 构建搜索路径
	wcscpy_s(searchPath, MAX_PATH, directoryPath);
	wcscat_s(searchPath, MAX_PATH, L"\\*.ini");

	// 开始查找第一个匹配的文件
	hFind = FindFirstFileW(searchPath, &findData);
	if (hFind == INVALID_HANDLE_VALUE) {
		*fileCount = 0;
		return NULL;
	}

	// 分配初始内存
	fileList = (wchar_t**)malloc(capacity * sizeof(wchar_t*));
	if (fileList == NULL) {
		FindClose(hFind);
		*fileCount = 0;
		return NULL;
	}

	// 枚举所有匹配的文件
	do {
		// 跳过 . 和 .. 目录
		if (wcscmp(findData.cFileName, L".") == 0 ||
			wcscmp(findData.cFileName, L"..") == 0) {
			continue;
		}

		// 只处理文件，不处理目录
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			// 计算完整路径长度
			size_t pathLen = wcslen(directoryPath) + wcslen(findData.cFileName) + 2;

			// 分配内存并构建完整路径
			fileList[count] = (wchar_t*)malloc(pathLen * sizeof(wchar_t));
			if (fileList[count] == NULL) {
				// 内存分配失败，清理已分配的内存
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

			// 如果数组已满，扩展容量
			if (count >= capacity) {
				capacity *= 2;
				wchar_t** newList = (wchar_t**)realloc(fileList, capacity * sizeof(wchar_t*));
				if (newList == NULL) {
					// 内存重新分配失败，清理已分配的内存
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

	// 关闭查找句柄
	FindClose(hFind);

	// 返回文件数量和文件列表
	*fileCount = count;
	return fileList;
}

// 释放文件列表内存
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

// 从INI文件读取配置信息
void ReadProgramFromIni(const wchar_t* filepath, ProgramInfo* config) {
	// 防止多次读取数据污染
	ZeroMemory(config, sizeof(ProgramInfo));
	// 读取Name
	GetPrivateProfileStringW(SECTION_NAME, L"Name", L"",
		config->name, sizeof(config->name) / sizeof(wchar_t),
		filepath);

	// 读取Command
	GetPrivateProfileStringW(SECTION_NAME, L"Command", L"",
		config->command, sizeof(config->command) / sizeof(wchar_t),
		filepath);

	// 读取Tags
	GetPrivateProfileStringW(SECTION_NAME, L"Tags", L"",
		config->tags, sizeof(config->tags) / sizeof(wchar_t),
		filepath);

}

// 从INI文件读取配置信息
void ReadSessionFromIni(const wchar_t* filepath, SessionInfo* config) {
	// 防止多次读取数据污染
	ZeroMemory(config, sizeof(SessionInfo));
	// 读取Name
	GetPrivateProfileStringW(SECTION_NAME, L"Name", L"",
		config->name, sizeof(config->name) / sizeof(wchar_t),
		filepath);

	// 读取HostName
	GetPrivateProfileStringW(SECTION_NAME, L"HostName", L"",
		config->hostName, sizeof(config->hostName) / sizeof(wchar_t),
		filepath);

	// 读取Port
	config->port = GetPrivateProfileIntW(SECTION_NAME, L"Port", 22, filepath);

	// 读取ConnectType
	GetPrivateProfileStringW(SECTION_NAME, L"ConnectType", L"",
		config->connectType, sizeof(config->connectType) / sizeof(wchar_t),
		filepath);

	// 读取Credential
	GetPrivateProfileStringW(SECTION_NAME, L"Credential", L"",
		config->credential, sizeof(config->credential) / sizeof(wchar_t),
		filepath);

	// 读取Tags
	GetPrivateProfileStringW(SECTION_NAME, L"Tags", L"",
		config->tags, sizeof(config->tags) / sizeof(wchar_t),
		filepath);
	// 自定义参数
	GetPrivateProfileStringW(SECTION_NAME, L"OtherParams", L"",
		config->otherParams, sizeof(config->otherParams) / sizeof(wchar_t),
		filepath);

}

// 从INI文件读取配置信息
void ReadCredentialFromIni(const wchar_t* filepath, CredentialInfo* config) {
	// 防止多次读取数据污染
	ZeroMemory(config, sizeof(CredentialInfo));
	// 读取Name
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

// 计算字符串的哈希值
unsigned int hash(const wchar_t* str, int capacity) {
	unsigned int hashval = 0;
	while (*str) {
		hashval = (*str++) + (hashval << 6) + (hashval << 16) - hashval;
	}
	return hashval % capacity;
}


// 初始化配置映射表
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

// 添加配置项
int addConfig(ConfigMap* map, const wchar_t* name, const wchar_t* username,
	const wchar_t* password, const wchar_t* privateKey) {
	if (map == NULL || name == NULL) return -1;

	// 计算哈希值
	unsigned int index = hash(name, map->capacity);

	// 检查是否已存在相同的键
	HashNode* current = map->table[index];
	while (current != NULL) {
		if (wcscmp(current->key, name) == 0) {
			// 更新已存在的项
			wcscpy_s(current->value.userName, sizeof(current->value.userName) / sizeof(wchar_t), username);
			wcscpy_s(current->value.password, sizeof(current->value.password) / sizeof(wchar_t), password);
			wcscpy_s(current->value.privateKey, sizeof(current->value.privateKey) / sizeof(wchar_t), privateKey);
			return 0;
		}
		// 索引键相同，但是非相同项，添加链表
		current = current->next;
	}

	// 创建新节点
	HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
	if (newNode == NULL) return -1;

	wcscpy_s(newNode->key, sizeof(newNode->key) / sizeof(wchar_t), name);
	wcscpy_s(newNode->value.name, sizeof(newNode->value.name) / sizeof(wchar_t), name);
	wcscpy_s(newNode->value.userName, sizeof(newNode->value.userName) / sizeof(wchar_t), username);
	wcscpy_s(newNode->value.password, sizeof(newNode->value.password) / sizeof(wchar_t), password);
	wcscpy_s(newNode->value.privateKey, sizeof(newNode->value.privateKey) / sizeof(wchar_t), privateKey);

	// 添加到链表头部
	newNode->next = map->table[index];
	map->table[index] = newNode;
	map->count++;

	return 0;
}

// 根据Name查找配置
CredentialInfo* findConfigByName(ConfigMap* map, const wchar_t* name) {
	if (map == NULL || name == NULL) return NULL;

	// 计算哈希值
	unsigned int index = hash(name, map->capacity);

	// 在链表中查找
	HashNode* current = map->table[index];
	while (current != NULL) {
		if (wcscmp(current->key, name) == 0) {
			return &(current->value);
		}
		current = current->next;
	}

	return NULL;
}

// 释放配置映射表内存
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
	// 释放桶数组
	free(map->table);
	// 释放哈希表结构体
	free(map);
}
