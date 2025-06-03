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

    wcex.cbSize = sizeof(WNDCLASSEX);

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
			// here we specify default properties of font shared by all editor instances
			// these could be later changed via "Choose font" dialog
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
			//wcscpy(g_tabEditorsInfo.editorFontProperties.lfFaceName, fontPropertyVal);
			g_tabEditorsInfo.editorFontHandle = CreateFontIndirectW(&(g_tabEditorsInfo.editorFontProperties));

			CreateToolBarTabControl(&g_tabEditorsInfo, hWnd);
			// 添加初始标签
			TCITEMW tie = { 0 };
			tie.mask = TCIF_TEXT;
			tie.pszText = (LPWSTR)L"标签 1";
			TabCtrl_InsertItem(hTabCtrl, 0, &tie);
		}
		break;
	case WM_SIZE:
		{
			// 调整标签控件和按钮大小
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

		/*selectTab(tabCtrlWinHandle, tabIndex);
		// enabling/disabling popup menu entries depending on number of tabs and index of selected tab
		EnableMenuItem(tabMenuHandle, ID_TAB_MOVETOLEFT, !(tabIndex > 0));
		EnableMenuItem(tabMenuHandle, ID_TAB_MOVETOLEFTMOST, !(tabIndex > 0));
		EnableMenuItem(tabMenuHandle, ID_TAB_MOVETORIGHT, !(tabIndex < (numTabs - 1)));
		EnableMenuItem(tabMenuHandle, ID_TAB_MOVETORIGHTMOST, !(tabIndex < (numTabs - 1)));
		*/
		TrackPopupMenu(tabMenuHandle, TPM_RIGHTBUTTON, absCursorPos.x, absCursorPos.y, 0, menuCommandProcessorWindowHandle, NULL);

		return 1;
	}
	}
	return 0;
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
	//wcscpy(tabCaptionFont.lfFaceName, &(L"MS Shell dlg")); // this font is used by dialog controls
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

void RemoveTab(HWND hTab, int currentTab) {
	int newTabItemsCount;
	int newSelectedTab;
	TabCtrl_DeleteItem(hTab, currentTab);
	newTabItemsCount = TabCtrl_GetItemCount(hTab);

	if (newTabItemsCount == 0) {
		AddNewTab(hTab);
	}
	else {
		// if last item was removed, select previous item, otherwise select next item
		newSelectedTab = (currentTab == newTabItemsCount) ? (currentTab - 1) : currentTab;
		TabCtrl_SetCurSel(hTab, newSelectedTab);
	}
}