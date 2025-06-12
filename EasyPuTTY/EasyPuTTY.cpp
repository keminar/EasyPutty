// EasyPuTTY.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "EasyPuTTY.h"

#define MAX_LOADSTRING 256
// 自定义定时
#define TIMER_ID_FOCUS 101
#define IDC_TABCONTROL 100

// 全局变量:
HINSTANCE g_appInstance;                        // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

HWND g_toolbarHandle;                          // 工具条
HWND g_mainWindowHandle;                       // 主窗体
int g_tabHitIndex;                             // 标签右键触发索引

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

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_EASYPUTTY, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

	// 注册快捷键来触发菜单命令
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EASYPUTTY));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
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
    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
	wcex.hIcon          = (HICON)LoadImage(hInstance, MAKEINTRESOURCEW(IDI_EASYPUTTY), IMAGE_ICON, 32, 32, 0);
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    //wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_EASYPUTTY);//不显示菜单
    wcex.lpszClassName  = szWindowClass;
	wcex.hIconSm        = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_EASYPUTTY), IMAGE_ICON, 16, 16, 0);

    return RegisterClassExW(&wcex);
}

//
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

	// 通过szWindowClass和前面注册的窗口类wcex关联起来
	g_mainWindowHandle = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!g_mainWindowHandle)
   {
      return FALSE;
   }

   ShowWindow(g_mainWindowHandle, nCmdShow);
   UpdateWindow(g_mainWindowHandle);

   return TRUE;
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
	case WM_CREATE:
	{
		// application properties
		wchar_t fontPropertyVal[LF_FACESIZE];
		StringCchCopyW(fontPropertyVal, sizeof(fontPropertyVal) / sizeof(wchar_t), L"Lucida console");
		// here we specify default properties of font shared by all editor instances
		g_tabWindowsInfo.editorFontProperties.lfHeight = -17; // this height seems fine
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
			MessageBoxW(NULL, L"创建工具条标签失败", L"提示", MB_OK);
			return 0;
		}
		else {
			// 获取标签的右键菜单
			g_tabWindowsInfo.tabMenuHandle = LoadMenuW(g_appInstance, MAKEINTRESOURCEW(IDM_TABMENU));
			g_tabWindowsInfo.tabMenuHandle = GetSubMenu(g_tabWindowsInfo.tabMenuHandle, 0); // we can't show top-level menu, we must use PopupMenu, which is a single child of this menu

			// 添加初始标签
			AddNewOverview(&g_tabWindowsInfo);
		}
	}
	return 0;
	case WM_SIZE:
	{
		// 跳过最小化处理，否则在putty中开screen最小化后最大化显示会变
		if (wParam == SIZE_MINIMIZED) {
			return 0;
		}
		// 调整标签控件和按钮大小
		if (g_toolbarHandle) {
			MoveWindow(g_toolbarHandle, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
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
		HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
		int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
		if (sel != -1) {
			TCCUSTOMITEM tabCtrlItemInfo = getTabItemInfo(tabCtrlWinHandle, sel);
			if (tabCtrlItemInfo.attachWindowHandle) {//解决句柄为NULL + RDW_ALLCHILDREN时，两个进程调整一个大小另一个也会刷新的问题
				RedrawWindow(tabCtrlItemInfo.attachWindowHandle, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
			}
		}
		// 在处理  WM_SIZ 期间调用 SetForegroundWindow 会不能调整大小, 通过定时器实现
		SetTimer(hWnd, TIMER_ID_FOCUS, 269, NULL);
		return 0;
	}
	break;
	case WM_TIMER:
		if (wParam == TIMER_ID_FOCUS) {
			KillTimer(hWnd, TIMER_ID_FOCUS); // 关闭计时器
			TCCUSTOMITEM tabCtrlItemInfo;
			HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
			int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
			if (sel != -1) {
				tabCtrlItemInfo.tcitemheader.mask = TCIF_PARAM;
				// retrieve information about tab control item with index i
				TabCtrl_GetItem(tabCtrlWinHandle, sel, &tabCtrlItemInfo);
				// tab焦点不能写在TabCtrl_GetCurSel前
				if (tabCtrlItemInfo.attachWindowHandle) {
					SetForegroundWindow(tabCtrlItemInfo.attachWindowHandle);
				}
				else {
					SetForegroundWindow(tabCtrlItemInfo.hostWindowHandle);
				}
			}
		}
		return 0;
    case WM_COMMAND: {
		HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
        int wmId = LOWORD(wParam);
        // 分析菜单选择:
        switch (wmId)
        {
		case IDM_OPEN: {
			AddNewOverview(&g_tabWindowsInfo);
			break;
		}
		case IDM_CLOSE: {
			int currentTab = TabCtrl_GetCurSel(tabCtrlWinHandle);
			RemoveTab(tabCtrlWinHandle, currentTab);
			break;
		}
		case ID_TAB_CLOSE: {//右键关闭
			RemoveTab(tabCtrlWinHandle, g_tabHitIndex);
			break;
		}
		case ID_TAB_MOVETOLEFT: {
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
		case ID_ENUM_WINDOW: {
			DialogBox(g_appInstance, MAKEINTRESOURCE(IDD_ENUMWIN), hWnd, ENUM);
			break;
		}
        case IDM_ABOUT:
            DialogBox(g_appInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
		case WM_GETMAINWINDOW:
			// 主窗口直接返回自身句柄
			return (LRESULT)hWnd;
		case ID_ENUM_ATTACH: {
			HWND attachHwnd = (HWND)lParam;
			AddAttachTab(&g_tabWindowsInfo, attachHwnd);
			return 0;
		}
		case ID_TAB_DETACH: {
			DetachTab(tabCtrlWinHandle, g_tabHitIndex);
			return 0;
		}
		case ID_LIST_ATTACH: { // 连接按钮点击
			// 获取输入框中的文本
			TCCUSTOMITEM tabCtrlItemInfo = { 0 };

			int sel = TabCtrl_GetCurSel(tabCtrlWinHandle);
			if (sel != -1) {
				tabCtrlItemInfo.tcitemheader.mask = TCIF_PARAM;
				TabCtrl_GetItem(tabCtrlWinHandle, sel, &tabCtrlItemInfo);
			}

			wchar_t* inputText = (wchar_t*)lParam;
			if (wcslen(inputText) == 0) {
				MessageBoxW(g_mainWindowHandle, L"请输入命令", L"提示", MB_OK | MB_ICONINFORMATION);
				return 0;
			}

			// 新建立
			HWND newHostWinHandle = createHostWindow(g_appInstance, (&g_tabWindowsInfo)->parentWinHandle);
			if (newHostWinHandle == NULL) {
				MessageBoxW(NULL, L"创建窗口失败", L"提示", MB_OK);
				return 0;
			}
			else {
				// 创建新的PuTTY标签页
				// 创建其他进程需要attachWinHandle打底，不然explorer测试有问题
				HWND puttyWindowHandle = createPuttyWindow(g_appInstance, newHostWinHandle, inputText);
				// 修改选项卡标题
				if (puttyWindowHandle && IsWindow(puttyWindowHandle)) {
					if (tabCtrlItemInfo.hostWindowHandle && IsWindow(tabCtrlItemInfo.hostWindowHandle)) {
						DestroyWindow(tabCtrlItemInfo.hostWindowHandle); // 会自动销毁所有子控件
					}
					DWORD  dwThreadId;
					GetWindowThreadProcessId(puttyWindowHandle, &dwThreadId);

					tabCtrlItemInfo.hostWindowHandle = newHostWinHandle;
					tabCtrlItemInfo.attachWindowHandle = puttyWindowHandle;
					tabCtrlItemInfo.attachProcessId = dwThreadId;

					// 要更新数据，窗口大小调整时才随动
					TabCtrl_SetItem(tabCtrlWinHandle, sel, &tabCtrlItemInfo);

					wchar_t attachTitle[256] = { 0 };
					wchar_t cutTitle[256] = { 0 };
					GetWindowTextW(puttyWindowHandle, attachTitle, _countof(attachTitle));
					TruncateString(attachTitle, cutTitle, 18);
					TCITEM tie = { 0 };
					tie.mask = TCIF_TEXT;
					tie.pszText = cutTitle;
					SendMessage(tabCtrlWinHandle, TCM_SETITEM, sel, (LPARAM)&tie);

					RECT rc;
					GetClientRect((&g_tabWindowsInfo)->parentWinHandle, &rc);
					rc = getTabRect(&g_tabWindowsInfo, rc);
					TabCtrl_AdjustRect(tabCtrlWinHandle, FALSE, &rc);
					setTabWindowPos(tabCtrlItemInfo.hostWindowHandle, puttyWindowHandle, rc);
					//重绘，部分软件需要，如cmd
					//RedrawWindow(tabCtrlItemInfo.attachWindowHandle, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);

					if (wcsstr(inputText, L"putty") != NULL) {
						// 按下 Ctrl 键
						keybd_event(VK_CONTROL, 0, 0, 0);
						// 按下 Space 键
						keybd_event(VK_SPACE, 0, 0, 0);
						// 释放 Space 键
						keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
						// 释放 Ctrl 键
						keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
					}
				}
				else {
					DestroyWindow(newHostWinHandle);
				}
			}
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
	case WM_LBUTTONDBLCLK: {
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
			TabCtrl_GetItemRect(tabCtrlWinHandle, count-1, &tabStripRect);
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
	case WM_DESTROY: {
		HWND tabCtrlWinHandle = (&g_tabWindowsInfo)->tabCtrlWinHandle;
		int tabItemsCount = TabCtrl_GetItemCount(tabCtrlWinHandle);
		for (int i = 0; i < tabItemsCount; i++) {
			RemoveTab(tabCtrlWinHandle, i);
		}

		PostQuitMessage(0);
		break;
	}	
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 假设hWnd是你想要聚焦的窗口句柄
void FocusWindow(HWND hWnd) {
	if (!hWnd) {
		return;
	}
	// 尝试将窗口带到前台并聚焦
	if (!SetForegroundWindow(hWnd)) {
		// 如果SetForegroundWindow失败，可以尝试其他方法
		BringWindowToTop(hWnd);
		SetFocus(hWnd);
	}
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

BOOL setTabWindowPos(HWND hostWinHandle, HWND attachWindowHandle, RECT rc) {
	//不能使用SetWindowPos窗口刷新会有问题，MoveWindow也不重绘
	// 新增：调整 PuTTY 窗口大小（若句柄有效）
	if (attachWindowHandle && IsWindow(attachWindowHandle)) {
		// 2次MoveWindow奇怪的解决了 cmd 进程反复attach 和detach时候的刷新问题，第一次可以是FALSE
		MoveWindow(attachWindowHandle, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
		int captionHeight = GetTitleBarHeightWithoutMenu(attachWindowHandle);
		// 这个要用TRUE
		MoveWindow(attachWindowHandle, 0, -captionHeight,
			rc.right - rc.left, rc.bottom - rc.top + captionHeight, TRUE);
		
	}
	return MoveWindow(hostWinHandle, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
}

LRESULT processTabNotification(HWND tabCtrlWinHandle, HMENU tabMenuHandle, HWND menuCommandProcessorWindowHandle, int code) {

	POINT cursorPos, absCursorPos;
	TCHITTESTINFO tabControlHitTestInfo;

	switch (code) {
	case TCN_SELCHANGING: {
			// Return 0 to allow the selection to change.
			return 0;
		}

	case TCN_SELCHANGE: {
		showWindowForSelectedTabItem(tabCtrlWinHandle, -1);

		SetTimer(g_mainWindowHandle, TIMER_ID_FOCUS, 1, NULL);
		return 1;
	}
	case NM_RCLICK: {
			GetCursorPos(&absCursorPos);
			cursorPos = absCursorPos;
			// since tab control is a child window itself (no self menu, no self border, ...) so it's client area corresponds to whole tab control window
			ScreenToClient(tabCtrlWinHandle, &cursorPos);
			tabControlHitTestInfo.pt = cursorPos;
			int tabIndex = TabCtrl_HitTest(tabCtrlWinHandle, &tabControlHitTestInfo);
			int numTabs = TabCtrl_GetItemCount(tabCtrlWinHandle);
			g_tabHitIndex = tabIndex;

			TCCUSTOMITEM tabCtrlItemInfo = getTabItemInfo(tabCtrlWinHandle, tabIndex);

			// enabling/disabling popup menu entries depending on number of tabs and index of selected tab
			EnableMenuItem(tabMenuHandle, ID_TAB_DETACH, !(tabCtrlItemInfo.attachProcessId > 0));
			EnableMenuItem(tabMenuHandle, ID_TAB_MOVETOLEFT, !(tabIndex > 0));
			EnableMenuItem(tabMenuHandle, ID_TAB_MOVETOLEFTMOST, !(tabIndex > 0));
			EnableMenuItem(tabMenuHandle, ID_TAB_MOVETORIGHT, !(tabIndex < (numTabs - 1)));
			EnableMenuItem(tabMenuHandle, ID_TAB_MOVETORIGHTMOST, !(tabIndex < (numTabs - 1)));
		
			TrackPopupMenu(tabMenuHandle, TPM_RIGHTBUTTON, absCursorPos.x, absCursorPos.y, 0, menuCommandProcessorWindowHandle, NULL);

			return 1;
		}
	}
	return 0;
}

// 只移动标签不修改选中
void moveTabToPosition(struct TabWindowsInfo* tabWindowsInfo, int tabIndex, int newPosition) {

	HWND tabCtrlWinHandle = tabWindowsInfo->tabCtrlWinHandle;
	TCCUSTOMITEM tabCtrlItemInfo;
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

// “枚举”框的消息处理程序。
INT_PTR CALLBACK ENUM(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		createEnum(g_appInstance, &g_tabWindowsInfo, hDlg);
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
			SendMessage(g_mainWindowHandle, WM_COMMAND, ID_ENUM_ATTACH, (LPARAM)lvItem.lParam);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_REFRESH) {
			HWND hListView = GetDlgItem(hDlg, ID_ENUM_VIEW);
			DestroyWindow(hListView);
			createEnum(g_appInstance, &g_tabWindowsInfo, hDlg);
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
    }
    return (INT_PTR)FALSE;
}

// 创建 TabControl 控件
void CreateToolBarTabControl(struct TabWindowsInfo *tabWindowsInfo, HWND parentWinHandle) {
	HWND tabCtrlWinHandle;
	LOGFONTW tabCaptionFont;
	HFONT tabCaptionFontHandle;

	tabWindowsInfo->tabWindowIdentifier = IDC_TABCONTROL;
	tabWindowsInfo->tabIncrementor = 0;
	tabWindowsInfo->parentWinHandle = parentWinHandle;

	// 创建 Toolbar
	g_toolbarHandle = CreateWindowExW(
		0, TOOLBARCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
		0, 0, 0, 0, parentWinHandle, (HMENU)IDR_MAIN_TOOLBAR, g_appInstance, NULL
	);

	// 设置 ImageList
	SendMessage(g_toolbarHandle, TB_SETIMAGELIST, 0, 0);
	// 定义按钮
	TBBUTTON tbButtons[] = {
		{ -1, IDM_OPEN,   TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"新建(&T)" },
		{ -1, IDM_CLOSE,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"关闭(&D)" },
		{ -1, ID_ENUM_WINDOW,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"窗口" }
	};

	// 添加按钮
	SendMessage(g_toolbarHandle, TB_ADDBUTTONS,
		sizeof(tbButtons) / sizeof(TBBUTTON), (LPARAM)&tbButtons);
	// 自动调整大小
	SendMessage(g_toolbarHandle, TB_AUTOSIZE, 0, 0);

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
	tabCaptionFont.lfHeight = -17; // this height seems fine
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
	wcscpy_s(tabCaptionFont.lfFaceName,_countof(tabCaptionFont.lfFaceName), L"MS Shell dlg"); // this font is used by dialog controls
	tabCaptionFontHandle = CreateFontIndirectW(&tabCaptionFont);

	SendMessageW(tabCtrlWinHandle, WM_SETFONT, (WPARAM)tabCaptionFontHandle, FALSE);
}

// 添加新标签
int AddNewTab(HWND tabCtrlWinHandle, int suffix) {
	TCCUSTOMITEM tabCtrlItemInfo;
	int count = TabCtrl_GetItemCount(tabCtrlWinHandle);
	wchar_t tabNameBuf[256];

	swprintf_s(tabNameBuf, L"新标签 %d", suffix);

	tabCtrlItemInfo.tcitemheader.mask = TCIF_TEXT | TCIF_IMAGE;
	tabCtrlItemInfo.tcitemheader.iImage = -1;
	tabCtrlItemInfo.tcitemheader.pszText = tabNameBuf;

	tabCtrlItemInfo.hostWindowHandle = 0;
	tabCtrlItemInfo.attachWindowHandle = 0;
	tabCtrlItemInfo.attachProcessId = 0;

	TabCtrl_InsertItem(tabCtrlWinHandle, count, &tabCtrlItemInfo);
	return count;
}

// 预览窗口
void AddNewOverview(struct TabWindowsInfo *tabWindowsInfo) {
	RECT rc;
	TCCUSTOMITEM tabCtrlItemInfo;
	int newTabIndex;
	HWND tabCtrlWinHandle;
	
	tabCtrlWinHandle = tabWindowsInfo->tabCtrlWinHandle;

	newTabIndex = AddNewTab(tabCtrlWinHandle, tabWindowsInfo->tabIncrementor + 1);
	HWND hostWindow = createHostWindow(g_appInstance, tabWindowsInfo->parentWinHandle);
	if (hostWindow == NULL) {
		TabCtrl_DeleteItem(tabCtrlWinHandle, newTabIndex);
		MessageBoxW(NULL, L"创建窗口失败", L"提示", MB_OK);
		return;
	}
	InitOverview(g_appInstance, tabWindowsInfo, hostWindow);
	// we need to associate window handle of rich edit with tab control item. We do that by using TabCtrl_SetItem with mask which specifies that only app data should be set
	tabCtrlItemInfo.tcitemheader.mask = TCIF_PARAM;
	tabCtrlItemInfo.hostWindowHandle = hostWindow;
	tabCtrlItemInfo.attachWindowHandle = 0;
	tabCtrlItemInfo.attachProcessId = 0;//必须再设置
	TabCtrl_SetItem(tabCtrlWinHandle, newTabIndex, &tabCtrlItemInfo);

	// 获取整个window区域
	GetClientRect(tabWindowsInfo->parentWinHandle, &rc);
	rc = getTabRect(tabWindowsInfo, rc);
	TabCtrl_AdjustRect(tabCtrlWinHandle, FALSE, &rc);
	setTabWindowPos(hostWindow, NULL, rc);
	selectTab(tabCtrlWinHandle, newTabIndex);
	(tabWindowsInfo->tabIncrementor)++;
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

TCCUSTOMITEM getTabItemInfo(HWND tabCtrlWinHandle, int i) {
	TCCUSTOMITEM tabCtrlItemInfo;

	// set mask so we are interested only in app data associated with tab item
	tabCtrlItemInfo.tcitemheader.mask = TCIF_PARAM;
	// retrieve information about tab control item with index i
	TabCtrl_GetItem(tabCtrlWinHandle, i, &tabCtrlItemInfo);
	return tabCtrlItemInfo;
}

 HWND getHostWindowForTabItem(HWND tabCtrlWinHandle, int i) {
	TCCUSTOMITEM tabCtrlItemInfo;

	tabCtrlItemInfo = getTabItemInfo(tabCtrlWinHandle, i);
	return tabCtrlItemInfo.hostWindowHandle;
}

// 删除标签
void RemoveTab(HWND tabCtrlWinHandle, int deleteTab) {
	int count;
	int newSelectedTab;
	int currentTab = TabCtrl_GetCurSel(tabCtrlWinHandle);

	TCCUSTOMITEM tabCtrlItemInfo;
	tabCtrlItemInfo.tcitemheader.mask = TCIF_PARAM;
	// retrieve information about tab control item with index i
	TabCtrl_GetItem(tabCtrlWinHandle, deleteTab, &tabCtrlItemInfo);

	// 最后一个预览不删除
	count = TabCtrl_GetItemCount(tabCtrlWinHandle);
	if (count == 1 && tabCtrlItemInfo.attachProcessId==0) {
		return;
	}

	// 删除标签
	TabCtrl_DeleteItem(tabCtrlWinHandle, deleteTab);

	// 销毁窗体
	if (tabCtrlItemInfo.hostWindowHandle) {
		DestroyWindow(tabCtrlItemInfo.hostWindowHandle);
	}
	if (tabCtrlItemInfo.attachWindowHandle) {
		DestroyWindow(tabCtrlItemInfo.attachWindowHandle);
	}
	// 检查进程是否关闭，超时强杀进程
	if (tabCtrlItemInfo.attachProcessId > 0) {
		DWORD dwExitCode = 0;
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
		}
		tabCtrlItemInfo.attachProcessId = 0;
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
	TCCUSTOMITEM tabCtrlItemInfo;
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


	// 调整每个标签页内容窗口的位置
	numTabs = TabCtrl_GetItemCount(tabCtrlWinHandle);
	for (i = 0; i < numTabs; i++) {
		tabCtrlItemInfo = getTabItemInfo(tabCtrlWinHandle, i);

		// 使用 clientRect 作为标签页内容的位置和大小
		setTabWindowPos(tabCtrlItemInfo.hostWindowHandle,
			tabCtrlItemInfo.attachWindowHandle,
			clientRect);
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
		MessageBoxW(NULL, L"无法创建宿主窗口", L"错误", MB_OK | MB_ICONERROR);
		return NULL;
	}
	return hostWindow;
}


void AddAttachTab(struct TabWindowsInfo *tabWindowsInfo, HWND attachHwnd) {
	RECT rc;
	TCCUSTOMITEM tabCtrlItemInfo;
	int newTabIndex;
	HWND tabCtrlWinHandle;

	tabCtrlWinHandle = tabWindowsInfo->tabCtrlWinHandle;

	newTabIndex = AddNewTab(tabCtrlWinHandle, tabWindowsInfo->tabIncrementor + 1);
	HWND hostWindow = createHostWindow(g_appInstance, tabWindowsInfo->parentWinHandle);
	if (hostWindow == NULL) {
		TabCtrl_DeleteItem(tabCtrlWinHandle, newTabIndex);
		MessageBoxW(NULL, L"创建窗口失败", L"提示", MB_OK);
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

	// 获取进程ID
	DWORD processId;
	GetWindowThreadProcessId(attachHwnd, &processId);

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
	setTabWindowPos(hostWindow, attachHwnd, rc);
	//重绘，部分软件需要，如cmd
	
	//RedrawWindow(attachHwnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

// 分离程序并删除标签
void DetachTab(HWND tabCtrlWinHandle, int indexTab) {
	int newTabItemsCount;
	int newSelectedTab;
	int currentTab = TabCtrl_GetCurSel(tabCtrlWinHandle);

	TCCUSTOMITEM tabCtrlItemInfo;
	tabCtrlItemInfo.tcitemheader.mask = TCIF_PARAM;
	TabCtrl_GetItem(tabCtrlWinHandle, indexTab, &tabCtrlItemInfo);

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
	if (tabCtrlItemInfo.attachWindowHandle) {
		// 获取子窗口当前位置和大小
		RECT rect;
		GetWindowRect(tabCtrlItemInfo.attachWindowHandle, &rect);
		SetParent(tabCtrlItemInfo.attachWindowHandle, NULL);

		if (!IsConsoleWindow(tabCtrlItemInfo.attachWindowHandle)) {
			LONG_PTR style = GetWindowLongPtr(tabCtrlItemInfo.attachWindowHandle, GWL_STYLE);
			style |= WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
			SetWindowLongPtr(tabCtrlItemInfo.attachWindowHandle, GWL_STYLE, style);
		}
		// 重新定位窗口，增加一些错位
		MoveWindow(tabCtrlItemInfo.attachWindowHandle,
			rect.left+30, rect.top+50,
			rect.right - rect.left, rect.bottom - rect.top,
			TRUE);
	}

	// 最后释放资源
	if (tabCtrlItemInfo.hostWindowHandle) {
		DestroyWindow(tabCtrlItemInfo.hostWindowHandle);
	}
}
