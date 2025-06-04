// EasyPuTTY.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "EasyPuTTY.h"

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE g_appInstance;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EASYPUTTY));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_EASYPUTTY);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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

   g_mainWindowHandle = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
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
			// application properties loaded from file
			wchar_t fontPropertyVal[LF_FACESIZE];
			StringCchCopyW(fontPropertyVal, sizeof(fontPropertyVal) / sizeof(wchar_t), L"Lucida console");
			// here we specify default properties of font shared by all editor instances
			g_tabEditorsInfo.editorFontProperties.lfHeight = -17; // this height seems fine
			g_tabEditorsInfo.editorFontProperties.lfWidth = 0;
			g_tabEditorsInfo.editorFontProperties.lfEscapement = 0;
			g_tabEditorsInfo.editorFontProperties.lfOrientation = 0;
			g_tabEditorsInfo.editorFontProperties.lfWeight = FW_NORMAL;
			g_tabEditorsInfo.editorFontProperties.lfItalic = FALSE;
			g_tabEditorsInfo.editorFontProperties.lfUnderline = FALSE;
			g_tabEditorsInfo.editorFontProperties.lfStrikeOut = FALSE;
			g_tabEditorsInfo.editorFontProperties.lfCharSet = ANSI_CHARSET;
			g_tabEditorsInfo.editorFontProperties.lfOutPrecision = OUT_DEFAULT_PRECIS;
			g_tabEditorsInfo.editorFontProperties.lfClipPrecision = CLIP_DEFAULT_PRECIS;
			g_tabEditorsInfo.editorFontProperties.lfQuality = DEFAULT_QUALITY;
			g_tabEditorsInfo.editorFontProperties.lfPitchAndFamily = DEFAULT_PITCH;
			wcscpy_s(g_tabEditorsInfo.editorFontProperties.lfFaceName, _countof(g_tabEditorsInfo.editorFontProperties.lfFaceName), fontPropertyVal);
			g_tabEditorsInfo.editorFontHandle = CreateFontIndirectW(&(g_tabEditorsInfo.editorFontProperties));

			CreateToolBarTabControl(&g_tabEditorsInfo, hWnd);

			if (g_tabEditorsInfo.tabCtrlWinHandle == NULL) {
				MessageBoxW(NULL, L"Error while creating main application window: could not create tab control", L"Note", MB_OK);
			}
			else {
				g_tabEditorsInfo.tabMenuHandle = LoadMenuW(g_appInstance, MAKEINTRESOURCEW(IDM_TABMENU));
				g_tabEditorsInfo.tabMenuHandle = GetSubMenu(g_tabEditorsInfo.tabMenuHandle, 0); // we can't show top-level menu, we must use PopupMenu, which is a single child of this menu

				// we want a single tab to be present in new window
				//createTabWithEditor(&g_tabEditorsInfo, TRUE);


				// 添加初始标签
				TCITEMW tie = { 0 };
				tie.mask = TCIF_TEXT;
				tie.pszText = (LPWSTR)L"标签 1";
				TabCtrl_InsertItem(hTabCtrl, 0, &tie);
			}
		}
		break;
	case WM_SIZE:
		{
			// 调整标签控件和按钮大小
			if (hToolbar) {
				MoveWindow(hToolbar, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
				// 自动调整大小
				SendMessage(hToolbar, TB_AUTOSIZE, 0, 0);
			}
			if (hTabCtrl) MoveWindow(hTabCtrl, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		}
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
			case IDM_OPEN:
				// 添加新标签
				AddNewTab(hTabCtrl);
				break;
			case IDM_CLOSE: {
					int currentTab = TabCtrl_GetCurSel(hTabCtrl);
					RemoveTab(hTabCtrl, currentTab);
				}
				break;
			case ID_TAB_CLOSE: {
					RemoveTab(hTabCtrl, g_tabHitIndex);
				}
				break;
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
            case IDM_ABOUT:
                DialogBox(g_appInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->hwndFrom == hTabCtrl) {
			return processTabNotification(g_tabEditorsInfo.tabCtrlWinHandle, g_tabEditorsInfo.tabMenuHandle, g_mainWindowHandle, ((LPNMHDR)lParam)->code);
		}
		break;
	case WM_LBUTTONDBLCLK: {
		// 处理鼠标双击事件(不包含标签上面的双击）
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);

		// 标准 C 语言中正确的 POINT 结构体初始化方式
		POINT pt = { x, y };

		// 获取标签区域
		RECT tabRect;
		GetClientRect(hTabCtrl, &tabRect);
		int count = TabCtrl_GetItemCount(hTabCtrl);
		if (count > 0) {
			// 获取最后一个标签项的矩形
			RECT tabStripRect = { 0 };
			TabCtrl_GetItemRect(hTabCtrl, count, &tabStripRect);
			tabRect.left = tabStripRect.right;
			tabRect.top = tabStripRect.top;
			if (PtInRect(&tabRect, pt)) {
				AddNewTab(hTabCtrl);
			}
		}
		break;
	}
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
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
		//showEditorForSelectedTabItem(tabCtrlWinHandle, -1);

		//SetTimer(g_mainWindowHandle, TIMER_ID_FOCUS, 1, NULL);
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

			/*selectTab(tabCtrlWinHandle, tabIndex);*/
			// enabling/disabling popup menu entries depending on number of tabs and index of selected tab
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

void selectTab(HWND tabCtrlWinHandle, int tabIndex) {
	TabCtrl_SetCurSel(tabCtrlWinHandle, tabIndex);
	//showEditorForSelectedTabItem(tabCtrlWinHandle, tabIndex);
}

// 只移动标签不修改选中
void moveTabToPosition(struct TabEditorsInfo* tabEditorsInfo, int tabIndex, int newPosition) {

	HWND tabCtrlWinHandle = tabEditorsInfo->tabCtrlWinHandle;
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
	int newTabItemsCount = TabCtrl_GetItemCount(g_tabEditorsInfo.tabCtrlWinHandle);
	if (g_tabHitIndex < newTabItemsCount - 1) {
		moveTabToPosition(&g_tabEditorsInfo, g_tabHitIndex, newTabItemsCount - 1);
	}
}

void selectedTabToRight() {
	int newTabItemsCount = TabCtrl_GetItemCount(g_tabEditorsInfo.tabCtrlWinHandle);
	if (g_tabHitIndex < newTabItemsCount - 1) {
		moveTabToPosition(&g_tabEditorsInfo, g_tabHitIndex, g_tabHitIndex + 1);
	}
}

void selectedTabToLeftmost() {
	int newTabItemsCount = TabCtrl_GetItemCount(g_tabEditorsInfo.tabCtrlWinHandle);
	if (g_tabHitIndex > 0) {
		moveTabToPosition(&g_tabEditorsInfo, g_tabHitIndex, 0);
	}
}

void selectedTabToLeft() {
	int newTabItemsCount = TabCtrl_GetItemCount(g_tabEditorsInfo.tabCtrlWinHandle);
	if (g_tabHitIndex > 0) {
		moveTabToPosition(&g_tabEditorsInfo, g_tabHitIndex, g_tabHitIndex - 1);
	}
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
void CreateToolBarTabControl(struct TabEditorsInfo *tabEditorsInfo, HWND parentWinHandle) {
	RECT rc;
	LOGFONTW tabCaptionFont;
	HFONT tabCaptionFontHandle;  // consider exposing this to TabEditorsInfo

	tabEditorsInfo->tabWindowIdentifier = 3000;  // just some value which I've chosen. This value is passed as param to CreateWindow for tabCtrl, and will be returned in WM_NOTIFY messages
	tabEditorsInfo->tabIncrementor = 0;

	tabEditorsInfo->parentWinHandle = parentWinHandle;

	// 创建 Toolbar
	hToolbar = CreateWindowExW(
		0, TOOLBARCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
		0, 0, 0, 0, parentWinHandle, (HMENU)IDR_MAIN_TOOLBAR, g_appInstance, NULL
	);

	// 设置 ImageList
	SendMessage(hToolbar, TB_SETIMAGELIST, 0, 0);
	// 定义按钮
	TBBUTTON tbButtons[] = {
		{ -1, IDM_OPEN,   TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"新建(&T)" },
		{ -1, IDM_CLOSE,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"关闭(&D)" }
	};

	// 添加按钮
	SendMessage(hToolbar, TB_ADDBUTTONS,
		sizeof(tbButtons) / sizeof(TBBUTTON), (LPARAM)&tbButtons);
	// 自动调整大小
	SendMessage(hToolbar, TB_AUTOSIZE, 0, 0);

	GetClientRect(parentWinHandle, &rc);
	// 创建标签控件
	hTabCtrl = CreateWindowW(
		WC_TABCONTROL, L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_FOCUSNEVER | TCS_HOTTRACK | TCS_BUTTONS | TCS_BOTTOM,
		0, 0, rc.right, rc.bottom, parentWinHandle, (HMENU)IDC_TABCONTROL, g_appInstance, NULL
	);
	if (hTabCtrl == NULL) {
		return ; // Error happened, and we don't handle it here, invoker should call GetLastError()
	}
	tabEditorsInfo->tabCtrlWinHandle = hTabCtrl;

	// We are going to store custom application data associated with each tab item. To achieve that, we need to specify once how many bytes do we need for app data
	TabCtrl_SetItemExtra(hTabCtrl, sizeof(TCCUSTOMITEM) - sizeof(TCITEMHEADER));

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

	SendMessageW(hTabCtrl, WM_SETFONT, (WPARAM)tabCaptionFontHandle, FALSE);
}

// 添加新标签
void AddNewTab(HWND hTab) {
	int count = TabCtrl_GetItemCount(hTab);
	wchar_t title[20];
	swprintf_s(title, L"新标签 %d", count + 1);

	TCITEMW tie = { 0 };
	tie.mask = TCIF_TEXT;
	tie.pszText = title;

	// 使用TabCtrl_InsertItemEx替代TabCtrl_InsertItem，减少重绘
	TabCtrl_InsertItem(hTab, count, &tie);
	TabCtrl_SetCurSel(hTab, count);
}

// 删除标签
void RemoveTab(HWND hTab, int deleteTab) {
	int newTabItemsCount;
	int newSelectedTab;
	int currentTab = TabCtrl_GetCurSel(hTabCtrl);

	TabCtrl_DeleteItem(hTab, deleteTab);
	newTabItemsCount = TabCtrl_GetItemCount(hTab);

	if (newTabItemsCount == 0) {
		AddNewTab(hTab);
	}
	else if (deleteTab == currentTab) { //如果删除项非选中项，不切换选中
		// if last item was removed, select previous item, otherwise select next item
		newSelectedTab = (currentTab == newTabItemsCount) ? (currentTab - 1) : currentTab;
		TabCtrl_SetCurSel(hTab, newSelectedTab);
	}
}