// EasyPuTTY.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "EasyPuTTY.h"

#define MAX_LOADSTRING 256
// 设置焦点定时
#define TIMER_ID_FOCUS 10
// 自定义消息：通知主窗口 Attach窗口 被点击
#define WM_ATTACH_CLICK (WM_USER + 1001)
#define WM_ATTACH_RCLICK (WM_USER + 1002)

// 确保这些常量已正确定义
#define WM_CLICKBIT (WM_USER + 1003)

// 全局变量:
HINSTANCE g_appInstance;                        // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

HWND g_toolbarHandle;                          // 工具条
HWND g_mainWindowHandle;                       // 主窗体
int g_tabHitIndex;                             // 标签右键触发索引
HWND g_hsearchEdit; //搜索框
int g_hsearchLastWordLen = 0;
UINT_PTR g_searchTimer = 0;       // 搜索框定时器ID

HWND g_debugWindow; // 日志窗口
// 静态变量用于标记窗口类是否已注册
static bool g_debugClassRegistered = false;

HHOOK g_hMouseHook = NULL;  // 钩子句柄
BOOL g_insideHook = FALSE;  // 标记是否正在处理钩子回调
HWND g_mouseHookWnd = NULL; // 当前拦截的putty窗体

// single global instance of TabWindowsInfo
struct TabWindowsInfo g_tabWindowsInfo;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// 初始化通用控件库并启用视觉样式
	INITCOMMONCONTROLSEX icex;
	// Initialize common controls.
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_TAB_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	// 初始化语言管理
	SetLangInstance(hInstance);
	InitLanguage();

	// 初始化全局字符串
	wcscpy_s(szTitle, MAX_LOADSTRING, GetString(IDS_APP_TITLE));
	//LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_EASYPUTTY, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	// 注册快捷键来触发菜单命令，对窗口焦点有要求不好用
	//HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EASYPUTTY));

	MSG msg;

	// 主消息循环:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		//if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		//{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		//}
	}

	return (int)msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	// since it is allocated on stack, we need to clear this memory before we use it
	memset(&wcex, 0, sizeof(wcex));

	wcex.cbSize = sizeof(WNDCLASSEX);

	// 支持双击消息
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCEW(IDI_EASYPUTTY), IMAGE_ICON, 32, 32, 0);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_EASYPUTTY);//不显示菜单
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_EASYPUTTY), IMAGE_ICON, 16, 16, 0);

	return RegisterClassExW(&wcex);
}

////
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	g_appInstance = hInstance; // 将实例句柄存储在全局变量中

	// 获取屏幕分辨率
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);  // 屏幕宽度
	int screenHeight = GetSystemMetrics(SM_CYSCREEN); // 屏幕高度

	int windowWidth = (int)(screenWidth * 0.8);
	int windowHeight = (int)(screenHeight * 0.8);
	if (windowWidth > 2 * windowHeight) {
		windowWidth = (int)(1.5*windowHeight);
	}
	else if (windowWidth < windowHeight) {
		windowHeight = windowWidth;
	}

	// 计算窗口左上角位置（居中显示）
	int windowX = (screenWidth - windowWidth) / 2;
	int windowY = (screenHeight - windowHeight) / 2;
	// 通过szWindowClass和前面注册的窗口类wcex关联起来
	g_mainWindowHandle = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS,
		windowX, windowY, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

	if (!g_mainWindowHandle)
	{
		return FALSE;
	}

	ShowWindow(g_mainWindowHandle, nCmdShow);
	UpdateWindow(g_mainWindowHandle);

	return TRUE;
}

// 剪贴板长度检查函数
int clipboardLen() {
	// 打开剪贴板
	if (!OpenClipboard(NULL)) {
		return 0;
	}

	int len = 0;
	// 检查剪贴板是否包含文本数据
	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		HANDLE hData = GetClipboardData(CF_UNICODETEXT);
		if (hData) {
			// 锁定内存并获取指针
			wchar_t* pszText = (wchar_t*)GlobalLock(hData);
			if (pszText) {
				len = wcslen(pszText);
				// 解锁内存
				GlobalUnlock(hData);
			}
		}
	}
	// 关闭剪贴板
	CloseClipboard();
	return len;
}

// 全局低级鼠标钩子回调（WH_MOUSE_LL）
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (g_insideHook) {
		return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
	}
	if (nCode >= 0) {
		// WH_MOUSE_LL 钩子的 lParam 是 MSLLHOOKSTRUCT*
		MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;

		// 1. 获取鼠标点击位置对应的窗口句柄
		HWND hWndUnderMouse = WindowFromPoint(pMouse->pt);
		 
		if (hWndUnderMouse && wParam == WM_LBUTTONDOWN) {
			DWORD dwThreadId = 0;
			GetWindowThreadProcessId(hWndUnderMouse, &dwThreadId);
			SendMessage(g_mainWindowHandle, WM_ATTACH_CLICK, wParam, (LPARAM)dwThreadId);
		}
		else if (hWndUnderMouse && wParam == WM_RBUTTONDOWN) {
			wchar_t className[256] = { 0 };
			if (GetClassNameW(hWndUnderMouse, className, 256)) {
				// 比较类名是否为"PuTTY"（大小写敏感）
				if (wcscmp(className, L"PuTTY") == 0) {
					g_mouseHookWnd = hWndUnderMouse;
					PostMessage(g_mainWindowHandle, WM_ATTACH_RCLICK, wParam, NULL);
					return 1;
				}
			}
		}
	}
	// 传递消息给下一个钩子
	return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}


// 注册快捷键
void registerAccel(HWND hWnd) {
	RegisterHotKey(hWnd, ID_HOTKEY_NEW, MOD_ALT, 'T');//新建
	RegisterHotKey(hWnd, ID_HOTKEY_CLOSE, MOD_ALT, 'D');//关闭
	RegisterHotKey(hWnd, ID_HOTKEY_WINDOW, MOD_ALT, 'W');//窗口
	RegisterHotKey(hWnd, ID_HOTKEY_SEARCH, MOD_ALT, 'F');//搜索
	RegisterHotKey(hWnd, ID_HOTKEY_CLONE, MOD_ALT, 'V');//克隆
}

// 注销快捷键
void unRegisterAccel(HWND hWnd) {
	UnregisterHotKey(hWnd, ID_HOTKEY_NEW);
	UnregisterHotKey(hWnd, ID_HOTKEY_CLOSE);
	UnregisterHotKey(hWnd, ID_HOTKEY_WINDOW);
	UnregisterHotKey(hWnd, ID_HOTKEY_SEARCH);
	UnregisterHotKey(hWnd, ID_HOTKEY_CLONE);
}

// 托盘图标创建函数
BOOL CreateTrayIcon(HWND hwnd)
{
	NOTIFYICONDATA nid = { 0 };

	// 根据系统版本设置正确的结构大小
#if (NTDDI_VERSION >= NTDDI_VISTA)
	nid.cbSize = NOTIFYICONDATA_V2_SIZE;
#else
	nid.cbSize = NOTIFYICONDATA_V1_SIZE;
#endif

	nid.hWnd = hwnd;
	nid.uID = IDI_EASYPUTTY;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_CLICKBIT; //自定义消息

	HICON hIcon = NULL;

	// 尝试加载指定尺寸的自定义图标
	hIcon = LoadIcon((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
		MAKEINTRESOURCE(IDI_EASYPUTTY));

	if (!hIcon) {
		return FALSE;
	}
	nid.hIcon = hIcon;

	// 设置提示文本
	wcscpy_s(nid.szTip, GetString(IDS_APP_TITLE));
	nid.dwState = NIS_SHAREDICON;//是否显示icon
	// 添加托盘图标
	BOOL result = Shell_NotifyIcon(NIM_ADD, &nid);
	// 释放图标资源
	DestroyIcon(hIcon);

	return result;
}

//销毁系统托盘图标 
void DestroyTrayIcon(HWND hwnd)
{
	NOTIFYICONDATA nid = { 0 };

	// 关键：根据系统版本设置正确的结构大小
#if (NTDDI_VERSION >= NTDDI_VISTA)
	nid.cbSize = NOTIFYICONDATA_V2_SIZE;
#else
	nid.cbSize = NOTIFYICONDATA_V1_SIZE;
#endif
	nid.uID = IDI_EASYPUTTY;
	nid.hWnd = hwnd;
	Shell_NotifyIcon(NIM_DELETE, &nid);
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ERASEBKGND: {
		// 直接返回TRUE表示我们已经处理了背景擦除
		// 这样可以避免系统默认的背景擦除操作，减少putty关闭和标签切换时的闪烁
		return TRUE;
	}
	case WM_CREATE:
	{
		wchar_t btnOk[64] = { 0 };
		// application properties
		wchar_t fontPropertyVal[LF_FACESIZE];
		StringCchCopyW(fontPropertyVal, sizeof(fontPropertyVal) / sizeof(wchar_t), L"Microsoft Sans Serif");
		//StringCchCopyW(fontPropertyVal, sizeof(fontPropertyVal) / sizeof(wchar_t), L"Lucida console");
		// here we specify default properties of font shared by all editor instances
		g_tabWindowsInfo.editorFontProperties.lfHeight = -16; //字体大小
		g_tabWindowsInfo.editorFontProperties.lfWidth = 0;
		g_tabWindowsInfo.editorFontProperties.lfEscapement = 0;
		g_tabWindowsInfo.editorFontProperties.lfOrientation = 0;
		g_tabWindowsInfo.editorFontProperties.lfWeight = FW_NORMAL;
		g_tabWindowsInfo.editorFontProperties.lfItalic = FALSE;
		g_tabWindowsInfo.editorFontProperties.lfUnderline = FALSE;
		g_tabWindowsInfo.editorFontProperties.lfStrikeOut = FALSE;
		g_tabWindowsInfo.editorFontProperties.lfCharSet = ANSI_CHARSET;
		g_tabWindowsInfo.editorFontProperties.lfOutPrecision = OUT_DEFAULT_PRECIS;
		g_tabWindowsInfo.editorFontProperties.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		g_tabWindowsInfo.editorFontProperties.lfQuality = DEFAULT_QUALITY;
		g_tabWindowsInfo.editorFontProperties.lfPitchAndFamily = DEFAULT_PITCH;
		wcscpy_s(g_tabWindowsInfo.editorFontProperties.lfFaceName, _countof(g_tabWindowsInfo.editorFontProperties.lfFaceName), fontPropertyVal);
		g_tabWindowsInfo.editorFontHandle = CreateFontIndirectW(&(g_tabWindowsInfo.editorFontProperties));

		// 创建工具条和标签
		CreateToolBarTabControl(&g_tabWindowsInfo, hWnd);

		if (g_tabWindowsInfo.tabCtrlWinHandle == NULL) {
			wcscpy_s(btnOk, _countof(btnOk), GetString(IDS_BTN_OK));
			MessageBoxW(NULL, GetString(IDS_MSG_CREATE_FAILED),
				btnOk, MB_OK);
			return 0;
		}
		else {
			// 获取标签的右键菜单
			g_tabWindowsInfo.tabMenuHandle = LoadMenuW(g_appInstance, MakeIntreSource(IDM_TABMENU, IDM_TABMENU_EN));
			g_tabWindowsInfo.tabMenuHandle = GetSubMenu(g_tabWindowsInfo.tabMenuHandle, 0); // we can't show top-level menu, we must use PopupMenu, which is a single child of this menu

			// 添加初始标签
			AddNewOverview(&g_tabWindowsInfo);
		}

		g_hMouseHook = SetWindowsHookEx(
			WH_MOUSE_LL,        // 全局低级鼠标钩子
			MouseHookProc,      // 回调函数
			GetModuleHandle(NULL),  // 当前模块句柄（无需 DLL）
			0                   // 0 表示监控所有线程
		);

		registerAccel(hWnd);
		return 0;
	}
	case WM_SIZE:
	{
		// 跳过最小化处理，否则在putty中开screen最小化后最大化显示会变
		if (wParam == SIZE_MINIMIZED) {
			return 0;
		}

		// 调整标签控件和按钮大小
		if (g_toolbarHandle) {
			// 控制窗口小的时候出横向滚动条
			//MoveWindow(g_toolbarHandle, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
			// 自动调整大小
			SendMessage(g_toolbarHandle, TB_AUTOSIZE, 0, 0);
		}
		RECT rc;
		// WM_SIZE params contain width and height of main window's client area
		// Since client area's left and top coordinates are both 0, having width and height gives us absolute coordinates of client's area
		// 值等于GetClientRect((&g_tabWindowsInfo)->parentWinHandle, &rc);
		SetRect(&rc, 0, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

		resizeTabControl(&g_tabWindowsInfo, rc);

		// 刷新当前标签
		/*HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
		int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
		if (sel != -1) {
			TCCUSTOMITEM tabCtrlItemInfo = { 0 };
			getTabItemInfo(tabCtrlWinHandle, sel, &tabCtrlItemInfo);
			if (tabCtrlItemInfo.attachWindowHandle) {//解决句柄为NULL + RDW_ALLCHILDREN时，两个进程调整一个大小另一个也会刷新的问题
				RedrawWindow(tabCtrlItemInfo.attachWindowHandle, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
			}
		}*/
		// 在处理  WM_SIZ 期间调用 SetForegroundWindow 会不能调整大小, 通过定时器实现
		SetTimer(hWnd, TIMER_ID_FOCUS, 500, NULL);
		return 0;
	}
	case WM_TIMER: {
		if (wParam == TIMER_ID_FOCUS) {
			KillTimer(hWnd, TIMER_ID_FOCUS); // 关闭计时器
			TCCUSTOMITEM tabCtrlItemInfo = { 0 };
			HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
			int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
			if (sel != -1) {
				getTabItemInfo(tabCtrlWinHandle, sel, &tabCtrlItemInfo);
				// tab焦点不能写在TabCtrl_GetCurSel前
				if (tabCtrlItemInfo.attachWindowHandle) {
					SetForegroundWindow(tabCtrlItemInfo.attachWindowHandle);
					for (int i = 0; i < 100; i++) {
						// 很重要，当为前台窗口才能设置焦点
						if (tabCtrlItemInfo.attachWindowHandle == GetForegroundWindow()) {
							// 设置焦点
							SetFocus(tabCtrlItemInfo.attachWindowHandle);
							break;
						}
						Sleep(30);
					}
				}
			}
		}
		return 0;
	}	
	case WM_ACTIVATE: {
		// 窗口切换激活，设置焦点
		switch (LOWORD(wParam)) {
		case WA_ACTIVE: { //（非鼠标点击，鼠标点击激活焦点不能失去，否则主窗体功能没法用）
			// 将现有窗口设置先置顶再取消
			SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			SetTimer(hWnd, TIMER_ID_FOCUS, 100, NULL);
			break;
		}
		}
		break;
	}
	case WM_ATTACH_CLICK: {//在attach程序上点击左键
		DWORD PID = (DWORD)lParam;
		HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
		int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
		if (sel != -1) {
			TCCUSTOMITEM tabCtrlItemInfo = { 0 };
			getTabItemInfo(tabCtrlWinHandle, sel, &tabCtrlItemInfo);
			if (tabCtrlItemInfo.attachWindowHandle && tabCtrlItemInfo.attachProcessId == PID) {
				//先置顶再取消，解决窗口层级问题
				SetWindowPos(hWnd, HWND_TOPMOST,  0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				SetWindowPos(hWnd, HWND_NOTOPMOST,0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			}
		}
		return 0;
	}
	case WM_ATTACH_RCLICK: {//putty右键粘贴
		HWND hWndUnderMouse  = g_mouseHookWnd;
		if (!hWndUnderMouse) {
			return 0;
		} 
		int len = clipboardLen();
		if (len <= 200) {
			g_insideHook = TRUE;  // 开始模拟前设置标记
			// 模拟发送右键按下消息
			SendMessage(hWndUnderMouse, WM_RBUTTONDOWN, MK_RBUTTON, NULL);
			// 发送右键释放消息（完成点击）
			SendMessage(hWndUnderMouse, WM_RBUTTONUP, 0, NULL);
			g_insideHook = FALSE; // 模拟完成后清除标记
			return 0;
		}

		wchar_t msg[256];
		FormatString(msg, 256, IDS_TIP_CLIPBOARD_LEN, len);
		int result = MessageBox(hWndUnderMouse,
			msg,
			GetString(IDS_TIP_CONFIRM_PASTE),
			MB_OKCANCEL | MB_ICONQUESTION);

		if (result == IDOK) {
			g_insideHook = TRUE;  // 开始模拟前设置标记
			// 模拟发送右键按下消息
			SendMessage(hWndUnderMouse, WM_RBUTTONDOWN, MK_RBUTTON, NULL);
			// 发送右键释放消息（完成点击）
			SendMessage(hWndUnderMouse, WM_RBUTTONUP, 0, NULL);
			g_insideHook = FALSE; // 模拟完成后清除标记
		}
		g_mouseHookWnd = NULL;
		return 0;
	}
	case WM_HOTKEY: {
		HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
		// wParam为注册时的id，判断是哪个快捷键
		switch (wParam) {
		case ID_HOTKEY_NEW:
			AddNewOverview(&g_tabWindowsInfo);
			break;
		case ID_HOTKEY_CLOSE: {
			int currentTab = TabCtrl_GetCurSel(tabCtrlWinHandle);
			RemoveTab(tabCtrlWinHandle, currentTab, FALSE);
			break;
		}
		case ID_HOTKEY_WINDOW:
			showDialogBox(g_appInstance, &g_tabWindowsInfo, MAKEINTRESOURCE(IDD_ENUMWIN), hWnd, ENUMProc);
			break;
		case ID_HOTKEY_SEARCH:
			SetFocus(g_hsearchEdit);
			break;
		case ID_HOTKEY_CLONE:
			cloneTab(tabCtrlWinHandle);
			break;
		}
		break;
	}
	case WM_COMMAND: {
		HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
		int wmId = LOWORD(wParam);
		// 分析菜单选择:
		switch (wmId)
		{
		case ID_SEARCH_BUTTON://搜索框清空
			if (HIWORD(wParam) == BN_CLICKED) {
				SetWindowText(g_hsearchEdit, L"");
				g_hsearchLastWordLen = 0;
				PerformSearch(g_hsearchEdit);
			}
			break;
		case IDM_OPEN: {//新增标签
			AddNewOverview(&g_tabWindowsInfo);
			break;
		}
		case IDM_SPLIT: {//四分屏
			createSplitWindow(g_appInstance, hWnd);
			break;
		}
		case ID_TAB_CLOSE: {//右键关闭当前鼠标位置标签
			RemoveTab(tabCtrlWinHandle, g_tabHitIndex, FALSE);
			break;
		}
		case ID_TAB_CLOSE_RIGHT: {
			HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
			int tabItemsCount = TabCtrl_GetItemCount(tabCtrlWinHandle);
			if (tabItemsCount <= g_tabHitIndex) {
				return 0;
			}
			for (int i = tabItemsCount - 1; i > g_tabHitIndex; i--) {
				RemoveTab(tabCtrlWinHandle, i, FALSE);
			}
			return 0;
		}
		case ID_TAB_CLOSE_OTHER: {
			HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
			int tabItemsCount = TabCtrl_GetItemCount(tabCtrlWinHandle);
			if (tabItemsCount < 1) {
				return 0;
			}
			for (int i = tabItemsCount - 1; i >= 0; i--) {
				if (i != g_tabHitIndex) {
					RemoveTab(tabCtrlWinHandle, i, FALSE);
				}
			}
			return 0;
		}
		case ID_TAB_CLOSE_ALL: {
			HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
			int tabItemsCount = TabCtrl_GetItemCount(tabCtrlWinHandle);
			if (tabItemsCount < 1) {
				return 0;
			}
			for (int i = tabItemsCount - 1; i >= 0; i--) {
				RemoveTab(tabCtrlWinHandle, i, FALSE);
			}
			return 0;
		}
		case ID_TAB_CLONE: {//克隆
			cloneTab(tabCtrlWinHandle);
			break;
		}
		case ID_TAB_RENAME: {//重命名弹窗
			showDialogBox(g_appInstance, &g_tabWindowsInfo, MakeIntreSource(IDD_RENAME, IDD_RENAME_EN), hWnd, RenameProc);
			break;
		}
		case ID_TAB_RENAME_GET: { // 获取旧标签名
			TCITEM tie = { 0 };
			tie.mask = TCIF_TEXT;         // 只获取文本属性
			tie.cchTextMax = 256;         // 缓冲区最大长度
			tie.pszText = (wchar_t*)lParam;   // 指向接收文本的缓冲区
			SendMessage(tabCtrlWinHandle, TCM_GETITEM, g_tabHitIndex, (LPARAM)&tie);
			return 0;
		}
		case ID_TAB_RENAME_DO: {//确认修改标签名
			wchar_t cutTitle[256] = { 0 };
			TruncateString((wchar_t*)lParam, cutTitle, 18);
			TCITEM tie = { 0 };
			tie.mask = TCIF_TEXT;
			tie.pszText = cutTitle;
			SendMessage(tabCtrlWinHandle, TCM_SETITEM, g_tabHitIndex, (LPARAM)&tie);
			break;
		}
		case ID_TAB_AUTO: { //attach进程自动关闭后回调更新标签
			HANDLE hProcess = (HANDLE)lParam;
			HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
			int currentTab;
			int count = TabCtrl_GetItemCount(tabCtrlWinHandle);
			int deleteTab = -1;
			TCCUSTOMITEM tabCtrlItemInfo = { 0 };
			for (int i = 0; i < count; i++) {
				 getTabItemInfo(tabCtrlWinHandle, i, &tabCtrlItemInfo);
				if (tabCtrlItemInfo.processHandle == hProcess) {
					deleteTab = i;

					ProcessUnRegisterClose(tabCtrlItemInfo.waitHandle, tabCtrlItemInfo.processHandle);
					// 先释放内存
					if (tabCtrlItemInfo.command != NULL) {
						delete[] tabCtrlItemInfo.command;
						tabCtrlItemInfo.command = NULL;
					}
					// 因为标签没马上删除，先更新标签数据, processHandle 下面判断还有用，先保留;
					tabCtrlItemInfo.waitHandle = NULL;
					tabCtrlItemInfo.attachProcessId = 0;
					tabCtrlItemInfo.attachWindowHandle = NULL;
					TabCtrl_SetItem(tabCtrlWinHandle, i, &tabCtrlItemInfo);
					break;
				}
			}
			if (deleteTab == -1) {
				return 0;
			}
			// 获取标签名
			wchar_t szText[MAX_PATH] = { 0 };
			TCITEM tie = { 0 };
			tie.mask = TCIF_TEXT;
			tie.cchTextMax = MAX_PATH;
			tie.pszText = szText;
			SendMessage(tabCtrlWinHandle, TCM_GETITEM, deleteTab, (LPARAM)&tie);
			swprintf_s(szText, _countof(szText), GetString(IDS_TAB_CLOSE_TIP), szText);
			MessageBox(hWnd, szText, GetString(IDS_TAB_CLOSE_TITLE), MB_OK);
			// 销毁标签, 因为删除标签后索引值会变化，所以用之前获取的deleteTab删除多个标签时先删除前面的
			// 再删除后面的此时索引值已经变化，会导致删除失败。解决办法是重新循环标签查询，总数也重新算
			count = TabCtrl_GetItemCount(tabCtrlWinHandle);
			for (int i = 0; i < count; i++) {
				getTabItemInfo(tabCtrlWinHandle, i, &tabCtrlItemInfo);
				if (tabCtrlItemInfo.processHandle == hProcess) {
					deleteTab = i;
					break;
				}
			}
			if (deleteTab == -1) {
				return 0;
			}
			// 要在删除前再获取一次当前选中值
			currentTab = TabCtrl_GetCurSel(tabCtrlWinHandle);
			TabCtrl_DeleteItem(tabCtrlWinHandle, deleteTab);
			if (tabCtrlItemInfo.hostWindowHandle && IsWindow(tabCtrlItemInfo.hostWindowHandle)) {
				DestroyWindow(tabCtrlItemInfo.hostWindowHandle);
			}
			// 获取最新标签总数
			count = TabCtrl_GetItemCount(tabCtrlWinHandle);
			if (count == 0) {
				AddNewOverview(&g_tabWindowsInfo);
			}
			else if (deleteTab == currentTab) {
				int newSelectedTab = (currentTab == count) ? (currentTab - 1) : currentTab;
				selectTab(tabCtrlWinHandle, newSelectedTab);
			}
			break;
		}
		case ID_TAB_REFRESH: {//右键刷新
			TCCUSTOMITEM tabCtrlItemInfo = { 0 };
			getTabItemInfo(tabCtrlWinHandle, g_tabHitIndex, &tabCtrlItemInfo);
			if (!tabCtrlItemInfo.attachWindowHandle) {
				HWND hListView = GetDlgItem(tabCtrlItemInfo.hostWindowHandle, ID_LIST_VIEW);
				SetListViewData(hListView);
			}
			break;
		}
		case ID_TAB_MOVETOLEFT: { // 移动标签
			selectedTabToLeft();
			return 0;
		}
		case ID_TAB_MOVETOLEFTMOST: {
			selectedTabToLeftmost();
			return 0;
		}
		case ID_TAB_MOVETORIGHT: {
			selectedTabToRight();
			return 0;
		}
		case ID_TAB_MOVETORIGHTMOST: {
			selectedTabToRightmost();
			return 0;
		}
		case IDM_ENUM_WINDOW: {
			showDialogBox(g_appInstance, &g_tabWindowsInfo, MakeIntreSource(IDD_ENUMWIN, IDD_ENUMWIN_EN), hWnd, ENUMProc);
			break;
		}
		case IDM_SETTING: {
			showDialogBox(g_appInstance, &g_tabWindowsInfo, MakeIntreSource(IDD_SETTING, IDD_SETTING_EN), hWnd, SettingProc);
			break;
		}
		case IDM_SESSION: {
			showDialogBox(g_appInstance, &g_tabWindowsInfo, MakeIntreSource(IDD_SESSION, IDD_SESSION_EN), hWnd, SessionProc);
			break;
		}
		case IDM_CREDENTIAL: {
			showDialogBox(g_appInstance, &g_tabWindowsInfo, MakeIntreSource(IDD_CREDENTIAL, IDD_CREDENTIAL_EN), hWnd, CredentialProc);
			break;
		}
		case IDM_PROGRAM: {
			showDialogBox(g_appInstance, &g_tabWindowsInfo, MakeIntreSource(IDD_PROGRAM, IDD_PROGRAM_EN), hWnd, ProgramProc);
			break;
		}
		case IDM_PAGEANT: {
			showDialogBox(g_appInstance, &g_tabWindowsInfo, MakeIntreSource(IDD_PAGEANT, IDD_PAGEANT_EN), hWnd, Pageant);
			break;
		}
		case IDM_PUTTYGEN: {
			wchar_t iniPath[MAX_PATH] = { 0 };
			wchar_t puttygen[MAX_PATH] = { 0 };
			// putty路径
			GetAppIni(iniPath, MAX_PATH);
			GetPrivateProfileStringW(SECTION_NAME, L"Puttygen", L"", puttygen, MAX_PATH, iniPath);
			if (puttygen[0] == L'\0') {
				wcscpy_s(puttygen, MAX_PATH, L".\\puttygen.exe");
			}
			startApp(puttygen, TRUE);
			break;
		}
		case IDM_DEBUG: {//日志窗口
			createDebugWindow();
			break;
		}
		case IDM_ABOUT:  //关于
			showDialogBox(g_appInstance, &g_tabWindowsInfo, MakeIntreSource(IDD_ABOUTBOX, IDD_ABOUTBOX_EN), hWnd, About);
			break;
		case IDM_EXIT://退出菜单
			DestroyWindow(hWnd);
			break;
		case WM_GETMAINWINDOW:
			// 主窗口直接返回自身句柄
			return (LRESULT)hWnd;
		case ID_ENUM_ATTACH: { // 嵌入程序
			HWND attachHwnd = (HWND)lParam;
			AddAttachTab(&g_tabWindowsInfo, attachHwnd);
			return 0;
		}
		case ID_TAB_DETACH: { //分离程序
			DetachTab(tabCtrlWinHandle, g_tabHitIndex, NULL);
			return 0;
		}
		case ID_TAB_DETACH_ALL: {//全分离
			HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
			int tabItemsCount = TabCtrl_GetItemCount(tabCtrlWinHandle);
			if (tabItemsCount < 1) {
				return 0;
			}
			for (int i = tabItemsCount - 1; i >=0; i--) {
				DetachTab(tabCtrlWinHandle, i, NULL);
			}
			return 0;
		}
		case ID_SPLIT_TOPLEFT: {
			SplitTab(g_tabHitIndex, 1);
			return 0;
		}
		case ID_SPLIT_TOPRIGHT: {
			SplitTab(g_tabHitIndex, 2);
			return 0;
		}
		case ID_SPLIT_BOTTOMLEFT: {
			SplitTab(g_tabHitIndex, 3);
			return 0;
		}
		case ID_SPLIT_BOTTOMRIGHT: {
			SplitTab(g_tabHitIndex, 4);
			return 0;
		}
		case ID_LIST_ATTACH: { // 点击启动程序并嵌入标签
			wchar_t msgCaption[MAX_PATH] = { 0 };
			int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
			NameCommand* selLine = (NameCommand*)lParam;
			if (wcslen(selLine->command) == 0) {
				wcscpy_s(msgCaption, _countof(msgCaption), GetString(IDS_MESSAGE_CAPTION));
				MessageBoxW(g_mainWindowHandle, GetString(IDS_NEED_COMMAND), msgCaption, MB_OK | MB_ICONINFORMATION);
				return 0;
			}
			openAttach(tabCtrlWinHandle, sel, selLine->name, selLine->command);
			return 0;
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	}
	case WM_NOTIFY: {
		HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
		if (((LPNMHDR)lParam)->hwndFrom == tabCtrlWinHandle) {
			return processTabNotification(g_tabWindowsInfo.tabCtrlWinHandle, g_tabWindowsInfo.tabMenuHandle, g_mainWindowHandle, ((LPNMHDR)lParam)->code);
		}
		break;
	}
	case WM_LBUTTONDBLCLK: {//双击开新标签
		// 处理鼠标双击事件(不包含标签上面的双击）
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);

		// 标准 C 语言中正确的 POINT 结构体初始化方式
		POINT pt = { x, y };

		// 获取标签区域
		RECT tabRect;
		HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
		GetClientRect(tabCtrlWinHandle, &tabRect);
		int count = TabCtrl_GetItemCount(tabCtrlWinHandle);
		if (count > 0) {
			// 获取最后一个标签项的矩形
			RECT tabStripRect = { 0 };
			RECT toolbarRect;
			TabCtrl_GetItemRect(tabCtrlWinHandle, count - 1, &tabStripRect);
			// 获取工具栏的屏幕坐标
			GetWindowRect(g_toolbarHandle, &toolbarRect);
			// 将工具栏坐标转换为父窗口的客户区坐标
			MapWindowPoints(HWND_DESKTOP, (&g_tabWindowsInfo)->parentWinHandle, (LPPOINT)&toolbarRect, 2);

			tabRect.left = tabStripRect.right;
			tabRect.top = tabStripRect.top;
			tabRect.bottom = toolbarRect.bottom + tabStripRect.bottom;
			if (PtInRect(&tabRect, pt)) {
				AddNewOverview(&g_tabWindowsInfo);
			}
		}
		break;
	}
	case WM_CLICKBIT: {//点击托盘图标
		switch (lParam)
		{
		case WM_LBUTTONUP://托盘图标还原窗口
			ShowWindow(hWnd, SW_SHOWNORMAL);
			::SetForegroundWindow(hWnd);
			DestroyTrayIcon(hWnd);
			break;
		default:
			break;
		}
		break;
	}
	case WM_CLOSE: {
		if (splitWindowAlive()) {
			// 隐藏窗口， 不关闭窗口是因为鼠标钩子还要用，关了就用不了了
			CreateTrayIcon(hWnd);
			ShowWindow(hWnd, SW_HIDE);
			return 0;
		}
		else {
			wchar_t confirm[MAX_PATH] = { 0 };
			wcscpy_s(confirm, _countof(confirm), GetString(IDS_TIP_CONFIRM_CLOSE));
			// 当用户点击关闭按钮时会触发WM_CLOSE消息
			int response = MessageBox(
				hWnd,
				GetString(IDS_TIP_CLOSE_APP),
				confirm,
				MB_YESNO | MB_ICONQUESTION
			);

			if (response == IDYES) {// 用户选择"是"
				// 销毁窗口
				DestroyWindow(hWnd);
			} else {
				// 用户选择"否"则不执行任何操作，窗口保持打开状态
				return 0;
			}
		}
	}
	case WM_DESTROY: {
		HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
		int tabItemsCount = TabCtrl_GetItemCount(tabCtrlWinHandle);
		if (tabItemsCount >= 1) {
			for (int i = tabItemsCount - 1; i >= 0; i--) {
				RemoveTab(tabCtrlWinHandle, i, TRUE);
			}
		}
		DestroyTrayIcon(hWnd);
		// 取消快捷键
		unRegisterAccel(hWnd);
		if (g_hMouseHook) {
			// 卸载鼠标钩子
			UnhookWindowsHookEx(g_hMouseHook);  // 必须卸载，否则可能导致进程崩溃
		}
		// 退出进程
		PostQuitMessage(0);
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 退出程序
void QuitEasyPutty() {
	PostMessage(g_mainWindowHandle, WM_CLOSE, 0, 0);
}

// 克隆标签
void cloneTab(HWND tabCtrlWinHandle) {
	TCCUSTOMITEM tabCtrlItemInfo = { 0 };
	getTabItemInfo(tabCtrlWinHandle, g_tabHitIndex, &tabCtrlItemInfo);
	// 检查克隆条件
	if (tabCtrlItemInfo.attachProcessId == 0) {
		return;
	}
	// 获取旧标签名
	wchar_t receivedText[256] = { 0 };  // 预先分配足够大的缓冲区
	TCITEM tie = { 0 };
	tie.mask = TCIF_TEXT;         // 只获取文本属性
	tie.cchTextMax = 256;         // 缓冲区最大长度
	tie.pszText = receivedText;   // 指向接收文本的缓冲区
	SendMessage(tabCtrlWinHandle, TCM_GETITEM, g_tabHitIndex, (LPARAM)&tie);
	// 新建标签
	wchar_t msgCaption[MAX_PATH] = { 0 };
	int newIndex = AddNewOverview(&g_tabWindowsInfo);
	if (newIndex == -1) {
		wcscpy_s(msgCaption, _countof(msgCaption), GetString(IDS_MESSAGE_CAPTION));
		MessageBoxW(g_mainWindowHandle, GetString(IDS_TAB_CLONE_FAIL), msgCaption, MB_OK | MB_ICONINFORMATION);
		return;
	}
	openAttach(tabCtrlWinHandle, newIndex, receivedText, tabCtrlItemInfo.command);
}

// 从overview新建立 或者克隆
void openAttach(HWND tabCtrlWinHandle, int selected, wchar_t* name, wchar_t* command) {
	TCCUSTOMITEM tabCtrlItemInfo = { 0 };
	HWND newHostWinHandle;
	HWND puttyWindowHandle;
	DWORD  dwThreadId;
	HANDLE hWait = NULL;
	HANDLE hProcess;
	size_t commandLen;
	RECT rc;

	newHostWinHandle = createHostWindow(g_appInstance, (&g_tabWindowsInfo)->parentWinHandle);
	if (newHostWinHandle == NULL) {
		wchar_t msgCaption[MAX_PATH] = { 0 };
		wcscpy_s(msgCaption, _countof(msgCaption), GetString(IDS_MESSAGE_CAPTION));
		MessageBoxW(NULL, GetString(IDS_TAB_CREATE_FAIL), msgCaption, MB_OK);
		return ;
	}
	else {
		getTabItemInfo(tabCtrlWinHandle, selected, &tabCtrlItemInfo);
		// 创建新的标签页，创建其他进程需要attachWinHandle打底，不然如explorer进程测试显示会有问题
		puttyWindowHandle = createPuttyWindow(g_appInstance, newHostWinHandle, command);
		if (puttyWindowHandle && IsWindow(puttyWindowHandle)) {
			if (tabCtrlItemInfo.hostWindowHandle && IsWindow(tabCtrlItemInfo.hostWindowHandle)) {
				DestroyWindow(tabCtrlItemInfo.hostWindowHandle); // 会自动销毁所有子控件
			}
			GetWindowThreadProcessId(puttyWindowHandle, &dwThreadId);
			hProcess = ProcessRegisterClose(dwThreadId, &hWait);

			// 输出调试日志
			LOG_INFO(L"hostWindow: %p", newHostWinHandle);

			tabCtrlItemInfo.hostWindowHandle = newHostWinHandle;
			tabCtrlItemInfo.attachWindowHandle = puttyWindowHandle;
			tabCtrlItemInfo.attachProcessId = dwThreadId;
			// 按照命令长度申请合适大小的内存，并存储字符
			commandLen = wcslen(command) + 1;
			if (tabCtrlItemInfo.command == NULL) {
				tabCtrlItemInfo.command = new wchar_t[commandLen];
			}
			wcscpy_s(tabCtrlItemInfo.command, commandLen, command);
			if (hProcess) {
				tabCtrlItemInfo.processHandle = hProcess;
				tabCtrlItemInfo.waitHandle = hWait;
			}

			// 要更新数据，窗口大小调整时才随动
			TabCtrl_SetItem(tabCtrlWinHandle, selected, &tabCtrlItemInfo);

			// 修改选项卡标题
			wchar_t cutTitle[256] = { 0 };
			TruncateString(name, cutTitle, 18);
			TCITEM tie = { 0 };
			tie.mask = TCIF_TEXT;
			tie.pszText = cutTitle;
			SendMessage(tabCtrlWinHandle, TCM_SETITEM, selected, (LPARAM)&tie);

			GetClientRect((&g_tabWindowsInfo)->parentWinHandle, &rc);
			rc = getTabRect(&g_tabWindowsInfo, rc);
			TabCtrl_AdjustRect(tabCtrlWinHandle, FALSE, &rc);
			setTabWindowPos(tabCtrlItemInfo.hostWindowHandle, puttyWindowHandle, rc, TRUE);

			if (wcsstr(command, L"putty") != NULL) {
				sendInputHotkey();
			}
		}
		else {
			DestroyWindow(newHostWinHandle);
		}
	}
}
// 注册回调
HANDLE ProcessRegisterClose(DWORD dwThreadId, HANDLE* hWait) {
	if (!hWait) {
		return NULL;
	}
	HANDLE hProcess = OpenProcess(
		SYNCHRONIZE | PROCESS_QUERY_INFORMATION,
		FALSE,
		dwThreadId
	);

	if (!hProcess) {
		return NULL;
	}
	BOOL result = RegisterWaitForSingleObject(
		hWait,                // 输出等待句柄
		hProcess,             // 等待的进程句柄
		ProcessEndCallback,   // 回调函数
		hProcess,             // 传递给回调的参数
		INFINITE,             // 等待超时时间（INFINITE为无限等待）
		WT_EXECUTEONLYONCE    // 只执行一次回调
	);
	if (!result) {
		CloseHandle(hProcess);
		return NULL;
	}
	return hProcess;
}

// 回收资源
void ProcessUnRegisterClose(HANDLE hWait, HANDLE hProcess) {
	// Microsoft 官方文档 指出在调用 UnregisterWaitEx 之前，不要关闭等待的句柄,如hProcess
	if (hWait) {
		UnregisterWaitEx(hWait, INVALID_HANDLE_VALUE);
	}
	if (hProcess) {
		CloseHandle(hProcess);
	}
}

// 进程结束时的回调函数
void CALLBACK ProcessEndCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired) {
	// 必须发消息回到主进程操作, 还得是异步消息
	PostMessage(g_mainWindowHandle, WM_COMMAND, ID_TAB_AUTO, LPARAM(lpParameter));
}

int GetTitleBarHeightWithoutMenu(HWND hWnd) {
	if (!IsWindow(hWnd)) return 0;
	RECT windowRect, clientRect;
	GetWindowRect(hWnd, &windowRect);        // 获取窗口在屏幕坐标系中的位置
	GetClientRect(hWnd, &clientRect);        // 获取客户区在窗口坐标系中的位置

	// 将客户区左上角坐标从窗口坐标转换为屏幕坐标
	POINT clientTopLeft = { clientRect.left, clientRect.top };
	ClientToScreen(hWnd, &clientTopLeft);

	// 计算标题栏高度（包含边框）
	int titleBarHeight = clientTopLeft.y - windowRect.top;
	return titleBarHeight;
}

BOOL setTabWindowPos(HWND hostWinHandle, HWND attachWindowHandle, RECT rc, BOOL refresh) {
	//不能使用SetWindowPos窗口刷新会有问题，MoveWindow也不重绘
	// 新增：调整 PuTTY 窗口大小（若句柄有效）
	if (attachWindowHandle && IsWindow(attachWindowHandle)) {
		// 2次MoveWindow奇怪的解决了 cmd 进程反复attach 和detach时候的刷新问题，第一次可以是FALSE
		MoveWindow(attachWindowHandle, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
		int captionHeight = GetTitleBarHeightWithoutMenu(attachWindowHandle);
		// 这个要用TRUE
		MoveWindow(attachWindowHandle, 0, -captionHeight+2,
			(rc.right - rc.left), (rc.bottom - rc.top + captionHeight), refresh);

	}
	return MoveWindow(hostWinHandle, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, refresh);
}

// tab事件
LRESULT processTabNotification(HWND tabCtrlWinHandle, HMENU tabMenuHandle, HWND menuCommandProcessorWindowHandle, int code) {

	POINT cursorPos, absCursorPos;
	TCHITTESTINFO tabControlHitTestInfo;

	switch (code) {
	case TCN_SELCHANGING: {
		// Return 0 to allow the selection to change.
		return 0;
	}

	case TCN_SELCHANGE: {//选中标签
		showWindowForSelectedTabItem(tabCtrlWinHandle, -1);

		SetTimer(g_mainWindowHandle, TIMER_ID_FOCUS, 1, NULL);
		return 1;
	}
	case NM_RCLICK: { //右键点击
		GetCursorPos(&absCursorPos);
		cursorPos = absCursorPos;
		// since tab control is a child window itself (no self menu, no self border, ...) so it's client area corresponds to whole tab control window
		ScreenToClient(tabCtrlWinHandle, &cursorPos);
		tabControlHitTestInfo.pt = cursorPos;
		int tabIndex = TabCtrl_HitTest(tabCtrlWinHandle, &tabControlHitTestInfo);
		int numTabs = TabCtrl_GetItemCount(tabCtrlWinHandle);
		g_tabHitIndex = tabIndex;

		TCCUSTOMITEM tabCtrlItemInfo = { 0 };
		getTabItemInfo(tabCtrlWinHandle, tabIndex, &tabCtrlItemInfo);

		// enabling/disabling popup menu entries depending on number of tabs and index of selected tab
		EnableMenuItem(tabMenuHandle, ID_TAB_DETACH, !(tabCtrlItemInfo.attachProcessId > 0));
		EnableMenuItem(tabMenuHandle, ID_TAB_REFRESH, (tabCtrlItemInfo.attachProcessId > 0));
		EnableMenuItem(tabMenuHandle, ID_TAB_MOVETOLEFT, !(tabIndex > 0));
		EnableMenuItem(tabMenuHandle, ID_TAB_MOVETOLEFTMOST, !(tabIndex > 0));
		EnableMenuItem(tabMenuHandle, ID_TAB_MOVETORIGHT, !(tabIndex < (numTabs - 1)));
		EnableMenuItem(tabMenuHandle, ID_TAB_MOVETORIGHTMOST, !(tabIndex < (numTabs - 1)));
		EnableMenuItem(tabMenuHandle, ID_TAB_CLONE, (tabCtrlItemInfo.command == NULL));

		TrackPopupMenu(tabMenuHandle, TPM_RIGHTBUTTON, absCursorPos.x, absCursorPos.y, 0, menuCommandProcessorWindowHandle, NULL);

		return 1;
	}
	}
	return 0;
}

// 只移动标签不修改选中
void moveTabToPosition(struct TabWindowsInfo* tabWindowsInfo, int tabIndex, int newPosition) {

	HWND tabCtrlWinHandle = tabWindowsInfo->tabCtrlWinHandle;
	TCCUSTOMITEM tabCtrlItemInfo = { 0 };
	wchar_t tabNameBuf[512];  // Temporary buffer for strings.
	tabCtrlItemInfo.tcitemheader.pszText = tabNameBuf;
	tabCtrlItemInfo.tcitemheader.cchTextMax = sizeof(tabNameBuf) / sizeof(wchar_t);

	// we want to get a copy of all the info for tab, so we need to specify all info item keys here
	tabCtrlItemInfo.tcitemheader.mask = TCIF_IMAGE | TCIF_PARAM | TCIF_RTLREADING | TCIF_STATE | TCIF_TEXT;
	// retrieve information about tab control item with tabIndex
	TabCtrl_GetItem(tabCtrlWinHandle, tabIndex, &tabCtrlItemInfo);
	// delete item on old position
	TabCtrl_DeleteItem(tabCtrlWinHandle, tabIndex);
	// insert new tab item into specified location
	TabCtrl_InsertItem(tabCtrlWinHandle, newPosition, &tabCtrlItemInfo); // content of tabControlItemInfo will be copied 
}

void selectedTabToRightmost() {
	int newTabItemsCount = TabCtrl_GetItemCount(g_tabWindowsInfo.tabCtrlWinHandle);
	if (g_tabHitIndex < newTabItemsCount - 1) {
		moveTabToPosition(&g_tabWindowsInfo, g_tabHitIndex, newTabItemsCount - 1);
	}
}

void selectedTabToRight() {
	int newTabItemsCount = TabCtrl_GetItemCount(g_tabWindowsInfo.tabCtrlWinHandle);
	if (g_tabHitIndex < newTabItemsCount - 1) {
		moveTabToPosition(&g_tabWindowsInfo, g_tabHitIndex, g_tabHitIndex + 1);
	}
}

void selectedTabToLeftmost() {
	int newTabItemsCount = TabCtrl_GetItemCount(g_tabWindowsInfo.tabCtrlWinHandle);
	if (g_tabHitIndex > 0) {
		moveTabToPosition(&g_tabWindowsInfo, g_tabHitIndex, 0);
	}
}

void selectedTabToLeft() {
	int newTabItemsCount = TabCtrl_GetItemCount(g_tabWindowsInfo.tabCtrlWinHandle);
	if (g_tabHitIndex > 0) {
		moveTabToPosition(&g_tabWindowsInfo, g_tabHitIndex, g_tabHitIndex - 1);
	}
}

// 获取当前本地时间，用于输入法回车上屏新和旧长度一致没更新的问题
int calcWordLen(int len) {
	SYSTEMTIME st;
	GetLocalTime(&st);
	return len + st.wSecond / 2 * 1000;
}

// 子类化后的窗口过程函数
LRESULT CALLBACK ToolbarProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	// 获取原始窗口过程
	WNDPROC originalProc = (WNDPROC)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	switch (message) {
	case WM_COMMAND: {
			// 低字是控件ID，高字是通知码
			UINT_PTR nID = LOWORD(wParam);
			UINT_PTR nCode = HIWORD(wParam);
			// 判断是否是目标编辑框的EN_CHANGE通知
			if (nID == ID_SEARCH_EDIT && nCode == EN_CHANGE) {
				// 收到内容变化，重置定时器
				if (g_searchTimer) {
					KillTimer(hWnd, g_searchTimer); // 清除旧定时器
				}
				// 启动新定时器（等待300毫秒）
				g_searchTimer = SetTimer(hWnd, 1, 300, NULL);
			}
		}
		break;
	case WM_TIMER: {
			wchar_t searchWord[256] = { 0 };
			// 定时器触发：视为输入稳定，处理内容
			KillTimer(hWnd, g_searchTimer); // 先销毁定时器
			g_searchTimer = 0;
			
			GetWindowText(g_hsearchEdit, searchWord, 256);
			int newLen = calcWordLen(lstrlen(searchWord));
			LOG_DEBUG(L"search: change g_hsearchLastWordLen=%d, wordlen=%d, newLen=%d", g_hsearchLastWordLen, lstrlen(searchWord), newLen);
			// 同长度刚查询过，不触发查询
			if (g_hsearchLastWordLen != newLen) {
				g_hsearchLastWordLen = newLen;
				PerformSearch(hWnd);
			}
		}
		break;
	}

	// 调用原始窗口过程处理其他消息
	return CallWindowProc(originalProc, hWnd, message, wParam, lParam);
}

LRESULT CALLBACK EditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	WNDPROC originalProc = (WNDPROC)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

	switch (message) {
		// 拦截字符消息（关键，很多时候响铃由此触发,如果不拦截按回车时会发生响铃）
	case WM_CHAR:
		if (wParam == VK_RETURN || wParam == 13) {// 13是回车的ASCII码
			wchar_t searchWord[256] = { 0 };
			GetWindowText(g_hsearchEdit, searchWord, 256);

			// 小狼毫输入法回车上屏
			int newLen = calcWordLen(lstrlen(searchWord));
			LOG_DEBUG(L"search: return g_hsearchLastWordLen=%d, wordlen=%d, newLen=%d", g_hsearchLastWordLen, lstrlen(searchWord), newLen);
			if (g_hsearchLastWordLen != newLen) {
				g_hsearchLastWordLen = newLen;
				PerformSearch(hWnd);
			}
			// 处理回车键
			TCCUSTOMITEM tabCtrlItemInfo = { 0 };
			int selected = TabCtrl_GetCurSel((&g_tabWindowsInfo)->tabCtrlWinHandle);
			getTabItemInfo((&g_tabWindowsInfo)->tabCtrlWinHandle, selected, &tabCtrlItemInfo);
			if (tabCtrlItemInfo.attachProcessId == 0) {
				// 获取 ListView 控件句柄
				HWND hListView = GetDlgItem(tabCtrlItemInfo.hostWindowHandle, ID_LIST_VIEW); // 或者通过其他方式获取

				// 获取行数
				int itemCount = ListView_GetItemCount(hListView);

				// 如果有项目，则选中第一行
				if (itemCount > 0) {
					// 设置第一行(索引0)为选中状态
					ListView_SetItemState(hListView, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

					// 确保选中的项可见
					ListView_EnsureVisible(hListView, 0, FALSE);
					SetFocus(hListView);
				}

			}
			return 0; // 阻止默认处理（包括响铃和输入回车符）
		}
		break;
	}

	// 调用原始窗口过程处理其他消息
	return CallWindowProc(originalProc, hWnd, message, wParam, lParam);
}

// 创建 TabControl 控件
void CreateToolBarTabControl(struct TabWindowsInfo *tabWindowsInfo, HWND parentWinHandle) {
	HWND tabCtrlWinHandle;
	LOGFONTW tabCaptionFont;
	HFONT tabCaptionFontHandle;
	int searchLeft = 700;
	int searchWidth = 300;
	// 为每个按钮文本定义独立的局部变量
	wchar_t btnTextCreate[64] = { 0 };
	wchar_t btnTextSplit[64] = { 0 };
	wchar_t btnTextWindow[64] = { 0 };
	wchar_t btnTextSession[64] = { 0 };
	wchar_t btnTextCredential[64] = { 0 };
	wchar_t btnTextPageant[64] = { 0 };
	wchar_t btnTextPuttygen[64] = { 0 };
	wchar_t btnTextSetting[64] = { 0 };
	wchar_t btnTextProgram[64] = { 0 };
	wchar_t btnTextDebug[64] = { 0 };
	wchar_t btnTextAbout[64] = { 0 };

	tabWindowsInfo->tabWindowIdentifier = IDC_TABCONTROL;
	tabWindowsInfo->tabIncrementor = 0;
	tabWindowsInfo->parentWinHandle = parentWinHandle;

	// 创建 Toolbar
	// 如果设置了TBSTYLE_FLAT，同时主窗体设置了WM_ERASEBKGND{return true;} 工具栏背景会变成黑色，所以不设置
	g_toolbarHandle = CreateWindowW(
		TOOLBARCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
		0, 0, 0, 0, parentWinHandle, (HMENU)IDR_MAIN_TOOLBAR, g_appInstance, NULL
	);
	WNDPROC originalToolbarProc = (WNDPROC)SetWindowLongPtr(g_toolbarHandle, GWLP_WNDPROC, (LONG_PTR)ToolbarProc);
	// 存储原始窗口过程，用于后续调用
	SetWindowLongPtrW(g_toolbarHandle, GWLP_USERDATA, (LONG_PTR)originalToolbarProc);
	// 设置 ImageList
	SendMessage(g_toolbarHandle, TB_SETIMAGELIST, 0, 0);


	// 从资源加载字符串到独立变量
	wcscpy_s(btnTextCreate, _countof(btnTextCreate), GetString(IDS_TOOLBAR_CREATE));
	wcscpy_s(btnTextSplit, _countof(btnTextSplit), GetString(IDS_TOOLBAR_SPLIT));
	wcscpy_s(btnTextWindow, _countof(btnTextWindow), GetString(IDS_TOOLBAR_WINDOW));
	wcscpy_s(btnTextSession, _countof(btnTextSession), GetString(IDS_TOOLBAR_SESSION));
	wcscpy_s(btnTextCredential, _countof(btnTextCredential), GetString(IDS_TOOLBAR_CREDENTIAL));
	wcscpy_s(btnTextPageant, _countof(btnTextPageant), GetString(IDS_TOOLBAR_PAGEANT));
	wcscpy_s(btnTextPuttygen, _countof(btnTextPuttygen), GetString(IDS_TOOLBAR_PUTTYGEN));
	wcscpy_s(btnTextSetting, _countof(btnTextSetting), GetString(IDS_TOOLBAR_SETTING));
	wcscpy_s(btnTextProgram, _countof(btnTextProgram), GetString(IDS_TOOLBAR_PROGRAM));
	wcscpy_s(btnTextDebug, _countof(btnTextDebug), GetString(IDS_TOOLBAR_DEBUG));
	wcscpy_s(btnTextAbout, _countof(btnTextAbout), GetString(IDS_TOOLBAR_ABOUT));

	// 定义按钮
	TBBUTTON tbButtons[] = {
		{ -1, IDM_OPEN,   TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)btnTextCreate },
		{ -1, IDM_SPLIT,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)btnTextSplit },
		{ -1, IDM_ENUM_WINDOW,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)btnTextWindow },
		{ -1, IDM_SESSION,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)btnTextSession },
		{ -1, IDM_CREDENTIAL,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)btnTextCredential },
		{ -1, IDM_PAGEANT,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)btnTextPageant },
		{ -1, IDM_PUTTYGEN,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)btnTextPuttygen },
		{ -1, IDM_SETTING,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)btnTextSetting },
		{ -1, IDM_PROGRAM,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)btnTextProgram },
		{ -1, IDM_DEBUG,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)btnTextDebug },
		{ -1, IDM_ABOUT,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)btnTextAbout }
	};
	//必须在调用 TB_ADDBUTTONS 之前设置TB_BUTTONSTRUCTSIZE 传递结构体大小，否则工具栏可能无法正确解析按钮数据。
	SendMessage(g_toolbarHandle, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
	// 添加按钮
	SendMessage(g_toolbarHandle, TB_ADDBUTTONS,
		sizeof(tbButtons) / sizeof(TBBUTTON), (LPARAM)&tbButtons);
	// 自动调整大小
	SendMessage(g_toolbarHandle, TB_AUTOSIZE, 0, 0);
	if (g_currentLang == LANG_EN) {
		searchLeft = 900;
	}
	// 创建搜索框和搜索按钮
	g_hsearchEdit = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		_T("EDIT"),
		_T(""),
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP | ES_WANTRETURN,
		searchLeft, 1, searchWidth, 30,
		g_toolbarHandle, (HMENU)ID_SEARCH_EDIT,
		g_appInstance, NULL
	);
	HWND searchButton = CreateWindowEx(
		0,
		_T("BUTTON"),
		GetString(IDS_BTN_CLEAR),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		searchLeft+ searchWidth, 1, 60, 30,
		g_toolbarHandle, (HMENU)ID_SEARCH_BUTTON,
		g_appInstance, NULL
	);
	WNDPROC originalProc = (WNDPROC)SetWindowLongPtr(g_hsearchEdit, GWLP_WNDPROC, (LONG_PTR)EditProc);
	// 存储原始窗口过程，用于后续调用
	SetWindowLongPtrW(g_hsearchEdit, GWLP_USERDATA, (LONG_PTR)originalProc);

	// 创建标签控件
	tabCtrlWinHandle = CreateWindowW(
		WC_TABCONTROL, L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_FOCUSNEVER | TCS_HOTTRACK,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parentWinHandle, (HMENU)tabWindowsInfo->tabWindowIdentifier, g_appInstance, NULL
	);
	if (tabCtrlWinHandle == NULL) {
		return; // Error happened, and we don't handle it here, invoker should call GetLastError()
	}

	tabWindowsInfo->tabCtrlWinHandle = tabCtrlWinHandle;

	// We are going to store custom application data associated with each tab item. To achieve that, we need to specify once how many bytes do we need for app data
	TabCtrl_SetItemExtra(tabCtrlWinHandle, sizeof(TCCUSTOMITEM) - sizeof(TCITEMHEADER));

	// Here we specify properties of font used in tab captions
	tabCaptionFont.lfHeight = -16;
	tabCaptionFont.lfWidth = 0;
	tabCaptionFont.lfEscapement = 0;
	tabCaptionFont.lfOrientation = 0;
	tabCaptionFont.lfWeight = FW_NORMAL;
	tabCaptionFont.lfItalic = FALSE;
	tabCaptionFont.lfUnderline = FALSE;
	tabCaptionFont.lfStrikeOut = FALSE;
	tabCaptionFont.lfCharSet = ANSI_CHARSET;
	tabCaptionFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	tabCaptionFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	tabCaptionFont.lfQuality = DEFAULT_QUALITY;
	tabCaptionFont.lfPitchAndFamily = DEFAULT_PITCH;
	wcscpy_s(tabCaptionFont.lfFaceName, _countof(tabCaptionFont.lfFaceName), L"MS Shell dlg"); // this font is used by dialog controls
	tabCaptionFontHandle = CreateFontIndirectW(&tabCaptionFont);

	SendMessageW(tabCtrlWinHandle, WM_SETFONT, (WPARAM)tabCaptionFontHandle, FALSE);
	SendMessageW(g_toolbarHandle, WM_SETFONT, (WPARAM)tabCaptionFontHandle, FALSE);
	SendMessageW(g_hsearchEdit, WM_SETFONT, (WPARAM)tabCaptionFontHandle, FALSE);
	SendMessageW(searchButton, WM_SETFONT, (WPARAM)tabCaptionFontHandle, FALSE);
}

// 添加新标签
int AddNewTab(HWND tabCtrlWinHandle, int suffix) {
	TCCUSTOMITEM tabCtrlItemInfo = { 0 };
	int count = TabCtrl_GetItemCount(tabCtrlWinHandle);
	wchar_t tabNameBuf[256];

	swprintf_s(tabNameBuf, GetString(IDS_TAB_UNTITLED), suffix);

	tabCtrlItemInfo.tcitemheader.mask = TCIF_TEXT | TCIF_IMAGE;
	tabCtrlItemInfo.tcitemheader.iImage = -1;
	tabCtrlItemInfo.tcitemheader.pszText = tabNameBuf;

	tabCtrlItemInfo.hostWindowHandle = NULL;
	tabCtrlItemInfo.attachWindowHandle = NULL;
	tabCtrlItemInfo.attachProcessId = 0;
	tabCtrlItemInfo.processHandle = NULL;
	tabCtrlItemInfo.waitHandle = NULL;
	tabCtrlItemInfo.command = NULL;

	TabCtrl_InsertItem(tabCtrlWinHandle, count, &tabCtrlItemInfo);
	return count;
}

// 预览窗口
int AddNewOverview(struct TabWindowsInfo *tabWindowsInfo) {
	RECT rc;
	TCCUSTOMITEM tabCtrlItemInfo = { 0 };
	int newTabIndex;
	HWND tabCtrlWinHandle;

	tabCtrlWinHandle = tabWindowsInfo->tabCtrlWinHandle;

	newTabIndex = AddNewTab(tabCtrlWinHandle, tabWindowsInfo->tabIncrementor + 1);
	HWND hostWindow = createHostWindow(g_appInstance, tabWindowsInfo->parentWinHandle);
	if (hostWindow == NULL) {
		TabCtrl_DeleteItem(tabCtrlWinHandle, newTabIndex);
		MessageBoxW(NULL, GetString(IDS_TAB_CREATE_FAIL), GetString(IDS_MESSAGE_CAPTION), MB_OK);
		return -1;
	}
	InitOverview(g_appInstance, tabWindowsInfo, hostWindow, g_hsearchEdit);
	// 先获取再更新
	getTabItemInfo(tabCtrlWinHandle, newTabIndex, &tabCtrlItemInfo);
	tabCtrlItemInfo.tcitemheader.mask = TCIF_PARAM;
	tabCtrlItemInfo.hostWindowHandle = hostWindow;
	TabCtrl_SetItem(tabCtrlWinHandle, newTabIndex, &tabCtrlItemInfo);

	// 获取整个window区域
	GetClientRect(tabWindowsInfo->parentWinHandle, &rc);
	rc = getTabRect(tabWindowsInfo, rc);
	TabCtrl_AdjustRect(tabCtrlWinHandle, FALSE, &rc);
	setTabWindowPos(hostWindow, NULL, rc, TRUE);
	selectTab(tabCtrlWinHandle, newTabIndex);
	(tabWindowsInfo->tabIncrementor)++;
	return newTabIndex;
}

void selectTab(HWND tabCtrlWinHandle, int tabIndex) {
	TabCtrl_SetCurSel(tabCtrlWinHandle, tabIndex);
	showWindowForSelectedTabItem(tabCtrlWinHandle, tabIndex);
}

// 切换到要显示的标签内容
void showWindowForSelectedTabItem(HWND tabCtrlWinHandle, int selected) {
	int iPage = selected < 0 ? TabCtrl_GetCurSel(tabCtrlWinHandle) : selected;
	int numTabs = TabCtrl_GetItemCount(tabCtrlWinHandle);
	int i;
	HWND hostHandle;
	for (i = 0; i < numTabs; i++) {
		hostHandle = getHostWindowForTabItem(tabCtrlWinHandle, i);
		if (i == iPage) {
			ShowWindow(hostHandle, SW_SHOW);
			SetFocus(hostHandle);
		}
		else {
			ShowWindow(hostHandle, SW_HIDE);
		}
	}
}

void getTabItemInfo(HWND tabCtrlWinHandle, int i, TCCUSTOMITEM* tabCtrlItemInfo) {
	if (i == -1) {
		return;
	}
	// set mask so we are interested only in app data associated with tab item
	tabCtrlItemInfo->tcitemheader.mask = TCIF_PARAM;
	// retrieve information about tab control item with index i
	TabCtrl_GetItem(tabCtrlWinHandle, i, tabCtrlItemInfo);
}

HWND getHostWindowForTabItem(HWND tabCtrlWinHandle, int i) {
	TCCUSTOMITEM tabCtrlItemInfo = { 0 };

	getTabItemInfo(tabCtrlWinHandle, i, &tabCtrlItemInfo);
	return tabCtrlItemInfo.hostWindowHandle;
}

// 删除标签
void RemoveTab(HWND tabCtrlWinHandle, int deleteTab, BOOL quit) {
	int count;
	int newSelectedTab;
	int currentTab = TabCtrl_GetCurSel(tabCtrlWinHandle);

	TCCUSTOMITEM tabCtrlItemInfo = { 0 };
	getTabItemInfo(tabCtrlWinHandle, deleteTab, &tabCtrlItemInfo);

	if (!quit) {
		// 最后一个预览不删除
		count = TabCtrl_GetItemCount(tabCtrlWinHandle);
		if (count == 1 && tabCtrlItemInfo.attachProcessId == 0) {
			return;
		}
	}

	// 删除标签
	TabCtrl_DeleteItem(tabCtrlWinHandle, deleteTab);
	if (tabCtrlItemInfo.attachWindowHandle && IsWindow(tabCtrlItemInfo.attachWindowHandle)) {
		//如果有attach了cmd进程 ，退出执行会发生(ntdll.dll)处引发的异常: 0xC0000005: 写入位置 0xCCCCCCD4 时发生访问冲突
		//暂时不知道如何解决，退出时if掉
		//目前好像已经解决访问冲突问题，去掉if
		//if (!quit) {
		ProcessUnRegisterClose(tabCtrlItemInfo.waitHandle, tabCtrlItemInfo.processHandle);
		//}

		// 销毁窗体
		DestroyWindow(tabCtrlItemInfo.attachWindowHandle);

		// 检查进程是否关闭，超时强杀进程
		// todo chrome不能关闭进程pid会影响所有标签窗口,vscode也有同样问题
		if (tabCtrlItemInfo.attachProcessId > 0) {
			/*DWORD dwExitCode = 0;
			// 打开进程，获取句柄
			HANDLE hProc = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, tabCtrlItemInfo.attachProcessId);
			if (hProc != NULL) {
				BOOL stoped = FALSE;
				int i;
				for (i = 0; i < 3; i++) {
					if (WaitForSingleObject(hProc, 50) == WAIT_OBJECT_0) {
						// 进程已结束
						stoped = TRUE;
						break;
					}
				}
				int j = i;
				if (!stoped) {
					//exe文件采用这种可以关闭
					DWORD dwExitCode = 0;
					// 获取子进程的退出码
					GetExitCodeProcess(hProc, &dwExitCode);
					TerminateProcess(hProc, dwExitCode);//终止进程
				}
				CloseHandle(hProc);
			}*/
			tabCtrlItemInfo.attachProcessId = 0;
		}
	}

	if (tabCtrlItemInfo.hostWindowHandle && IsWindow(tabCtrlItemInfo.hostWindowHandle)) {
		DestroyWindow(tabCtrlItemInfo.hostWindowHandle);
	}
	// 释放内存
	if (tabCtrlItemInfo.command != NULL) {
		delete[] tabCtrlItemInfo.command;
	}
	if (quit) {
		return;
	}

	// 标签切换
	count = TabCtrl_GetItemCount(tabCtrlWinHandle);
	if (count == 0) {
		AddNewOverview(&g_tabWindowsInfo);
	}
	else if (deleteTab == currentTab) { //如果删除项非选中项，不切换选中
		// if last item was removed, select previous item, otherwise select next item
		newSelectedTab = (currentTab == count) ? (currentTab - 1) : currentTab;
		selectTab(tabCtrlWinHandle, newSelectedTab);
	}
}

// 含标签条大小位置
RECT getTabRect(struct TabWindowsInfo *tabWindowsInfo, RECT rc) {
	RECT toolbarRect;
	RECT tabRect = rc;
	// 获取工具栏的屏幕坐标
	GetWindowRect(g_toolbarHandle, &toolbarRect);

	// 将工具栏坐标转换为父窗口的客户区坐标
	MapWindowPoints(HWND_DESKTOP, tabWindowsInfo->parentWinHandle, (LPPOINT)&toolbarRect, 2);

	// 计算工具栏高度
	int toolbarHeight = toolbarRect.bottom - toolbarRect.top;

	// 调整标签控件位置到工具栏下方
	tabRect.top = toolbarRect.bottom;
	tabRect.bottom = rc.bottom;

	return tabRect;
}

// Resize tab container so it fits provided RECT
// RECT is specified in parent window's client coordinates
HRESULT resizeTabControl(struct TabWindowsInfo *tabWindowsInfo, RECT rc) {
	int numTabs, i;
	TCCUSTOMITEM tabCtrlItemInfo = { 0 };
	RECT clientRect;       // 标签控件客户区的位置和大小

	HWND tabCtrlWinHandle = tabWindowsInfo->tabCtrlWinHandle;

	rc = getTabRect(tabWindowsInfo, rc);
	clientRect = rc;
	// 计算标签控件的客户区（不包括标签栏）
	TabCtrl_AdjustRect(tabCtrlWinHandle, FALSE, &clientRect);

	// 设置标签条
	if (!SetWindowPos(tabCtrlWinHandle,
		HWND_TOP,
		rc.left,
		rc.top,    // 从工具栏下方开始
		rc.right - rc.left,
		clientRect.top - rc.top,      // 使用正确的高度
		SWP_DEFERERASE | SWP_NOREPOSITION | SWP_NOOWNERZORDER))
		return E_FAIL;

	int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
	
	// 调整每个标签页内容窗口的位置
	numTabs = TabCtrl_GetItemCount(tabCtrlWinHandle);
	for (i = 0; i < numTabs; i++) {
		getTabItemInfo(tabCtrlWinHandle, i, &tabCtrlItemInfo);

		BOOL refresh = FALSE;
		if (sel == i) {
			refresh = TRUE;
		}
		// 使用 clientRect 作为标签页内容的位置和大小
		setTabWindowPos(tabCtrlItemInfo.hostWindowHandle,
			tabCtrlItemInfo.attachWindowHandle,
			clientRect, refresh);
	}

	return S_OK;
}

HWND createHostWindow(HINSTANCE hInstance, HWND parentWindow) {
	// 创建宿主窗口
	HWND hostWindow = CreateWindowExW(
		0,
		L"Static",
		L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0,
		800, 600,
		parentWindow,
		NULL,
		hInstance,
		NULL
	);

	if (!hostWindow) {
		MessageBoxW(NULL, GetString(IDS_HOSTWINDOW_FAIL), GetString(IDS_MESSAGE_CAPTION), MB_OK | MB_ICONERROR);
		return NULL;
	}
	return hostWindow;
}

// 调试窗口过程
LRESULT CALLBACK DebugWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{ 
	switch (msg)
	{
	case WM_CREATE:
	{
		// 在窗口中创建独立的编辑框控件
		HWND hDebugEdit = CreateWindowExW(
			0,
			L"EDIT",
			L"",      // 初始文本为空
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | WS_BORDER,
			0, 0, 0, 0,
			hWnd,
			(HMENU)ID_DEBUG_EDIT,
			GetModuleHandle(NULL),
			NULL
		);

		// 设置编辑框字体
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		SendMessage(hDebugEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

		setEditHwnd(hDebugEdit);
		break;
	}
	case WM_SIZE:
	{
		HWND hDebugEdit = GetDlgItem(hWnd, ID_DEBUG_EDIT);
		if (hDebugEdit)
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			MoveWindow(hDebugEdit, 5, 0, rect.right-5, rect.bottom-5, TRUE);
		}
		break;
	}
	case WM_DESTROY:
		setEditHwnd(NULL);
		g_debugWindow = NULL;
		return 0;
	}

	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// 调试窗口
void createDebugWindow() {
	const wchar_t* CLASS_NAME = L"DebugWindowClass";
	if (g_debugWindow) {
		// 检查窗口是否处于最小化状态
		if (IsIconic(g_debugWindow)) {
			// 还原窗口（从最小化状态恢复）
			ShowWindow(g_debugWindow, SW_RESTORE);
		}
		SetForegroundWindow(g_debugWindow);
		return;
	}
	// 确保窗口类只注册一次（首次调用时注册）
	if (!g_debugClassRegistered) {
		WNDCLASS wc = { 0 };
		wc.lpfnWndProc = DebugWindowProc;
		wc.hInstance = g_appInstance;
		wc.lpszClassName = CLASS_NAME;
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		
		// 尝试注册窗口类
		ATOM atom = RegisterClass(&wc);
		DWORD error = GetLastError();

		// 检查是否注册成功或类已存在
		if (!atom && error != ERROR_CLASS_ALREADY_EXISTS) {
			MessageBoxW(NULL, GetString(IDS_REGISTER_WNDCLASS_FAIL), GetString(IDS_MESSAGE_CAPTION), MB_OK | MB_ICONERROR);
			return;
		}
		g_debugClassRegistered = true; // 标记为已注册（无论本次是否实际注册）
	}

	g_debugWindow = CreateWindowExW(
		0,
		CLASS_NAME,
		GetString(IDS_DEBUG_TITLE),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		900, 600,
		NULL,
		NULL,
		GetModuleHandle(NULL),
		NULL
	);

	if (!g_debugWindow) {
		MessageBoxW(NULL, GetString(IDS_LOG_WINDOW_FAIL), GetString(IDS_MESSAGE_CAPTION), MB_OK | MB_ICONERROR);
		return ;
	}
}

// 附加窗口
void AddAttachTab(struct TabWindowsInfo *tabWindowsInfo, HWND attachHwnd) {
	RECT rc;
	TCCUSTOMITEM tabCtrlItemInfo = { 0 };
	int newTabIndex;
	HWND tabCtrlWinHandle;

	tabCtrlWinHandle = tabWindowsInfo->tabCtrlWinHandle;

	newTabIndex = AddNewTab(tabCtrlWinHandle, tabWindowsInfo->tabIncrementor + 1);
	HWND hostWindow = createHostWindow(g_appInstance, tabWindowsInfo->parentWinHandle);
	if (hostWindow == NULL) {
		TabCtrl_DeleteItem(tabCtrlWinHandle, newTabIndex);
		MessageBoxW(NULL, GetString(IDS_TAB_CREATE_FAIL), GetString(IDS_MESSAGE_CAPTION), MB_OK);
		return;
	}
	// 嵌入窗口到宿主窗口
	SetParent(attachHwnd, hostWindow);
	// cmd设置样式会有问题
	if (!IsConsoleWindow(attachHwnd)) {
		LONG_PTR style = GetWindowLongPtr(attachHwnd, GWL_STYLE);
		style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
		SetWindowLongPtr(attachHwnd, GWL_STYLE, style);
	}
	getTabItemInfo(tabCtrlWinHandle, newTabIndex, &tabCtrlItemInfo);
	// 获取进程ID
	DWORD processId;
	GetWindowThreadProcessId(attachHwnd, &processId);

	HANDLE hWait = NULL;
	HANDLE hProcess = ProcessRegisterClose(processId, &hWait);
	if (hProcess) {
		tabCtrlItemInfo.processHandle = hProcess;
		tabCtrlItemInfo.waitHandle = hWait;
	}

	tabCtrlItemInfo.tcitemheader.mask = TCIF_PARAM;
	tabCtrlItemInfo.hostWindowHandle = hostWindow;
	tabCtrlItemInfo.attachWindowHandle = attachHwnd;
	tabCtrlItemInfo.attachProcessId = processId;
	TabCtrl_SetItem(tabCtrlWinHandle, newTabIndex, &tabCtrlItemInfo);
	selectTab(tabCtrlWinHandle, newTabIndex);
	(tabWindowsInfo->tabIncrementor)++;

	// 更新标题
	wchar_t attachTitle[256] = { 0 };
	wchar_t cutTitle[256] = { 0 };
	GetWindowTextW(attachHwnd, attachTitle, _countof(attachTitle));
	TruncateString(attachTitle, cutTitle, 18);
	TCITEM tie = { 0 };
	tie.mask = TCIF_TEXT;
	tie.pszText = cutTitle;
	SendMessage(tabCtrlWinHandle, TCM_SETITEM, newTabIndex, (LPARAM)&tie);

	// 获取整个window区域
	GetClientRect(tabWindowsInfo->parentWinHandle, &rc);
	rc = getTabRect(tabWindowsInfo, rc);
	TabCtrl_AdjustRect(tabCtrlWinHandle, FALSE, &rc);
	setTabWindowPos(hostWindow, attachHwnd, rc, TRUE);
	//重绘，部分软件需要，如cmd

	//RedrawWindow(attachHwnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

// 分离程序并删除标签
void DetachTab(HWND tabCtrlWinHandle, int indexTab, HWND newParent) {
	int newTabItemsCount;
	int newSelectedTab;
	int currentTab = TabCtrl_GetCurSel(tabCtrlWinHandle);

	TCCUSTOMITEM tabCtrlItemInfo = { 0 };
	getTabItemInfo(tabCtrlWinHandle, indexTab, &tabCtrlItemInfo);
	if (tabCtrlItemInfo.attachProcessId == 0) {
		return;
	}
	if (!tabCtrlItemInfo.attachWindowHandle || !IsWindow(tabCtrlItemInfo.attachWindowHandle)) {
		return;
	}

	// 先关闭标签
	TabCtrl_DeleteItem(tabCtrlWinHandle, indexTab);
	newTabItemsCount = TabCtrl_GetItemCount(tabCtrlWinHandle);
	if (newTabItemsCount == 0) {
		AddNewOverview(&g_tabWindowsInfo);
	}
	else if (indexTab == currentTab) { //如果删除项非选中项，不切换选中
		// if last item was removed, select previous item, otherwise select next item
		newSelectedTab = (currentTab == newTabItemsCount) ? (currentTab - 1) : currentTab;
		selectTab(tabCtrlWinHandle, newSelectedTab);
	}

	// 后分离窗口，这样新窗口在前台
	RECT rect;
	// 获取子窗口当前位置和大小
	GetWindowRect(tabCtrlItemInfo.attachWindowHandle, &rect);
	// 先修改样式，因为分离后设置样式可能无效导致部分软件显示异常
	if (!IsConsoleWindow(tabCtrlItemInfo.attachWindowHandle)) {
		LONG_PTR style = GetWindowLongPtr(tabCtrlItemInfo.attachWindowHandle, GWL_STYLE);
		style |= WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		SetWindowLongPtr(tabCtrlItemInfo.attachWindowHandle, GWL_STYLE, style);
	}
	// 去掉回调函数
	ProcessUnRegisterClose(tabCtrlItemInfo.waitHandle, tabCtrlItemInfo.processHandle);
	// 最后分离
	SetParent(tabCtrlItemInfo.attachWindowHandle, newParent);
	if (newParent == NULL) {
		int diff = indexTab % 10 * 30;
		// 重新定位窗口，增加一些错位
		MoveWindow(tabCtrlItemInfo.attachWindowHandle,
			rect.left + 30 + diff, rect.top + 50 + diff,
			rect.right - rect.left, rect.bottom - rect.top,
			TRUE);
	}
	
	// 最后释放资源
	if (tabCtrlItemInfo.hostWindowHandle && IsWindow(tabCtrlItemInfo.hostWindowHandle)) {
		DestroyWindow(tabCtrlItemInfo.hostWindowHandle);
	}
}

void SplitTab(int g_tabHitIndex, int pos) {
	HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
	HWND splitHwnd = createSplitWindow(g_appInstance, g_mainWindowHandle);
	if (!splitHwnd) {
		return;
	}
	// 先获取窗口
	TCCUSTOMITEM tabCtrlItemInfo = { 0 };
	getTabItemInfo(tabCtrlWinHandle, g_tabHitIndex, &tabCtrlItemInfo);
	// 分离窗口
	DetachTab(tabCtrlWinHandle, g_tabHitIndex, splitHwnd);
	// 写入分屏编号
	if (tabCtrlItemInfo.attachWindowHandle && IsWindow(tabCtrlItemInfo.attachWindowHandle)) {
		insertSplitWindow(tabCtrlItemInfo.attachWindowHandle, pos);
	}
}

// 执行搜索功能的函数
void PerformSearch(HWND hWnd) {
	LOG_DEBUG(L"search: search");
	HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
	int currentTab = TabCtrl_GetCurSel(tabCtrlWinHandle);

	TCCUSTOMITEM tabCtrlItemInfo = { 0 };
	getTabItemInfo(tabCtrlWinHandle, currentTab, &tabCtrlItemInfo);
	if (tabCtrlItemInfo.attachProcessId > 0) {
		return;
	}
	HWND hListView = GetDlgItem(tabCtrlItemInfo.hostWindowHandle, ID_LIST_VIEW);
	SetListViewData(hListView);
}