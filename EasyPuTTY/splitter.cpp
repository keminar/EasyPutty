#include "splitter.h"
#include "attach.h"
#include "logs.h"

static HINSTANCE g_appInstance;

HWND g_hWndMain;                // 主窗口句柄
HWND g_hWndHSplit;              // 水平分隔条
HWND g_hWndVSplitTop;           // 顶部垂直分隔条
HWND g_hWndVSplitBottom;        // 底部垂直分隔条
HWND g_hScrollTop, g_hScrollBottom; // 同步滚动条
HWND puttyHandle1, puttyHandle2, puttyHandle3, puttyHandle4;

RECT g_rcMain;                  // 主窗口客户区矩形
int g_nHSplitPos = 450;         // 水平分隔条位置
int g_nVSplitTopPos = 600;      // 顶部垂直分隔条位置
int g_nVSplitBottomPos = 600;   // 底部垂直分隔条位置

// 分隔条位置比例
double g_dHSplitRatio = 0.5;     // 水平分隔条位置比例
double g_dVSplitTopRatio = 0.5;  // 顶部垂直分隔条位置比例
double g_dVSplitBottomRatio = 0.5; // 底部垂直分隔条位置比例

const int SPLITTER_SIZE = 5;    // 分隔条粗细
const int SCROLLBAR_WIDTH = 20; // 滚动条宽度

// 确保窗口类只注册一次（首次调用时注册）
static bool g_splitMainClassRegistered = false;
static bool g_splitLineClassRegistered = false;
static bool g_scrollbarClassRegistered = false;

int g_nDragging = 0;            // 0=未拖动, 1=水平, 2=顶部垂直, 3=底部垂直, 4=顶部滚动条, 5=底部滚动条
#define TIMER_ID_RESIZE 11      //定时器

// 注册窗口类
void registerClass() {
	if (!g_splitMainClassRegistered) {
		WNDCLASSEX wc = { 0 };
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC;
		wc.lpfnWndProc = SplitWindowProc;
		wc.hInstance = g_appInstance;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszClassName = L"SplitScreenClass";

		// 尝试注册窗口类
		ATOM atom = RegisterClassEx(&wc);
		DWORD error = GetLastError();

		// 检查是否注册成功或类已存在
		if (!atom && error != ERROR_CLASS_ALREADY_EXISTS) {
			MessageBoxW(NULL, GetString(IDS_REGISTER_WNDCLASS_FAIL), GetString(IDS_MESSAGE_CAPTION), MB_OK | MB_ICONERROR);
			return;
		}
		g_splitMainClassRegistered = true; // 标记为已注册（无论本次是否实际注册）
	}
	if (!g_splitLineClassRegistered) {
		WNDCLASSEX splitterWC = { 0 };
		splitterWC.cbSize = sizeof(WNDCLASSEX);
		splitterWC.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		splitterWC.lpfnWndProc = SplitterProc;
		splitterWC.hInstance = g_appInstance;
		splitterWC.hCursor = LoadCursor(NULL, IDC_ARROW);
		splitterWC.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
		splitterWC.lpszClassName = _T("SplitterLineClass");


		// 尝试注册窗口类
		ATOM atom = RegisterClassEx(&splitterWC);
		DWORD error = GetLastError();

		// 检查是否注册成功或类已存在
		if (!atom && error != ERROR_CLASS_ALREADY_EXISTS) {
			MessageBoxW(NULL, GetString(IDS_REGISTER_WNDCLASS_FAIL), GetString(IDS_MESSAGE_CAPTION), MB_OK | MB_ICONERROR);
			return;
		}
		g_splitLineClassRegistered = true; // 标记为已注册（无论本次是否实际注册）
	}
	if (!g_scrollbarClassRegistered) {
		WNDCLASSEX scrollWC = { 0 };
		scrollWC.cbSize = sizeof(WNDCLASSEX);
		scrollWC.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		scrollWC.lpfnWndProc = ScrollbarProc;
		scrollWC.hInstance = g_appInstance;
		scrollWC.hCursor = LoadCursor(NULL, IDC_ARROW);
		scrollWC.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		scrollWC.lpszClassName = _T("CustomScrollbar");

		// 尝试注册窗口类
		ATOM atom = RegisterClassEx(&scrollWC);
		DWORD error = GetLastError();

		// 检查是否注册成功或类已存在
		if (!atom && error != ERROR_CLASS_ALREADY_EXISTS) {
			MessageBoxW(NULL, GetString(IDS_REGISTER_WNDCLASS_FAIL), GetString(IDS_MESSAGE_CAPTION), MB_OK | MB_ICONERROR);
			return;
		}
		g_scrollbarClassRegistered = true; // 标记为已注册（无论本次是否实际注册）
	}
}

// 分屏主窗口
HWND createSplitWindow(HINSTANCE hInstance, HWND appWindow) {
	g_appInstance = hInstance;
	if (g_hWndMain) {
		// 检查窗口是否处于最小化状态
		if (IsIconic(g_hWndMain)) {
			// 还原窗口（从最小化状态恢复）
			ShowWindow(g_hWndMain, SW_RESTORE);
		}
		SetForegroundWindow(g_hWndMain);
		return g_hWndMain;
	}
	// 确保窗口类只注册一次（首次调用时注册）
	registerClass();

	g_hWndMain = CreateWindowExW(
		0,
		L"SplitScreenClass",
		GetString(IDS_SPLIT_TITLE),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1200, 900,
		NULL,
		NULL,
		g_appInstance,
		NULL
	);
	if (!g_hWndMain) {
		MessageBoxW(NULL, GetString(IDS_HOSTWINDOW_FAIL), GetString(IDS_MESSAGE_CAPTION), MB_OK | MB_ICONERROR);
		return NULL;
	}
	return g_hWndMain;
}

// 分屏窗口过程
LRESULT CALLBACK SplitWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	if (msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP)
	{
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
	}
	switch (msg)
	{
	case WM_ERASEBKGND: {
		// 直接返回TRUE表示我们已经处理了背景擦除
		// 这样可以避免系统默认的背景擦除操作，减少putty关闭和标签切换时的闪烁
		return TRUE;
	}
	case WM_CREATE:
	{
		CreateChildWindows(hWnd);
		break;
	}
	case WM_SIZE: {
		GetClientRect(g_hWndMain, &g_rcMain);
		// 调整分隔条位置
		if (g_rcMain.right > 0 && g_rcMain.bottom > 0)
		{
			g_nHSplitPos = (int)(g_dHSplitRatio * g_rcMain.bottom);
			g_nVSplitTopPos = (int)(g_dVSplitTopRatio * g_rcMain.right);
			g_nVSplitBottomPos = (int)(g_dVSplitBottomRatio * g_rcMain.right);
		}

		ArrangeWindows();
		break;
	}
	case WM_CAPTURECHANGED:
		g_nDragging = 0;
		break;
	case WM_MOUSEMOVE:
		// 处理拖动
		if (g_nDragging > 0 && g_nDragging <= 3)
		{
			// 更新分隔条位置
			switch (g_nDragging)
			{
			case 1: // 水平分隔条
				g_nHSplitPos = pt.y;
				g_nHSplitPos = min(max(g_nHSplitPos, 10), g_rcMain.bottom - 10);
				g_dHSplitRatio = (double)g_nHSplitPos / g_rcMain.bottom;
				break;
			case 2: // 顶部垂直分隔条
				g_nVSplitTopPos = pt.x;
				g_nVSplitTopPos = min(max(g_nVSplitTopPos, 10), g_rcMain.right - 10 - SCROLLBAR_WIDTH);
				g_dVSplitTopRatio = (double)g_nVSplitTopPos / g_rcMain.right;
				break;
			case 3: // 底部垂直分隔条
				g_nVSplitBottomPos = pt.x;
				g_nVSplitBottomPos = min(max(g_nVSplitBottomPos, 10), g_rcMain.right - 10 - SCROLLBAR_WIDTH);
				g_dVSplitBottomRatio = (double)g_nVSplitBottomPos / g_rcMain.right;
				break;
			}
			// 通过定时器减少窗口重绘次数，让画面变化更平滑
			SetTimer(hWnd, TIMER_ID_RESIZE, 10, NULL);
			return 0;
		}
		// 鼠标悬停时改变光标 - 增强分隔条可交互性提示
		else
		{
			int splitter = GetSplitterAtPoint(pt);
			if (splitter == 1)
			{
				SetCursor(LoadCursor(NULL, IDC_SIZENS));
			}
			else if (splitter == 2 || splitter == 3)
			{
				SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			}
			else
			{
				SetCursor(LoadCursor(NULL, IDC_ARROW));
			}
		}
		break;
	case WM_TIMER: {
		if (wParam == TIMER_ID_RESIZE) {
			KillTimer(hWnd, TIMER_ID_RESIZE); // 关闭计时器
			ArrangeWindows();
		}
		return 0;
	}
	case WM_LBUTTONDOWN:
		// 开始拖动
		g_nDragging = GetSplitterAtPoint(pt);
		if (g_nDragging > 0)
		{
			SetCapture(hWnd);
			if (g_nDragging == 1)
			{
				SetCursor(LoadCursor(NULL, IDC_SIZENS));
			}
			else
			{
				SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			}
			return 0;
		}
		break;

	case WM_LBUTTONUP:
		// 结束拖动
		if (g_nDragging > 0)
		{
			g_nDragging = 0;
			ReleaseCapture();
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			return 0;
		}
		break;
	case WM_DESTROY: {
		g_hWndMain = NULL;
		// 重置比例
		g_dHSplitRatio = 0.5;
		g_dVSplitTopRatio = 0.5;
		g_dVSplitBottomRatio = 0.5;
		puttyHandle1 = NULL;
		puttyHandle2 = NULL;
		puttyHandle3 = NULL;
		puttyHandle4 = NULL;
		return 0;
	}
	}

	return DefWindowProcW(hWnd, msg, wParam, lParam);
}


// 嵌入PuTTY窗口到主窗口
void insertSplitWindow(HWND puttyHwnd, int pos) {
	if (pos == 2){
		puttyHandle2 = puttyHwnd;
	} else if (pos == 3) {
		puttyHandle3 = puttyHwnd;
	}
	else if (pos == 4) {
		puttyHandle4 = puttyHwnd;
	}
	else {
		puttyHandle1 = puttyHwnd;
	}
	ArrangeWindows();
}

// 创建子窗口
void CreateChildWindows(HWND parentWindow)
{
	g_hWndMain = parentWindow;
	/*
	puttyHandle1 = createPuttyWindow(g_appInstance, g_hWndMain, L"cmd");
	MoveWindow(puttyHandle1, 0, 0, 600, 450, TRUE);
	
	puttyHandle2 = createPuttyWindow(g_appInstance, g_hWndMain, L"cmd");
	MoveWindow(puttyHandle2, 0, 0, 600, 450, TRUE);
	
	puttyHandle3 = createPuttyWindow(g_appInstance, g_hWndMain, L"cmd");
	MoveWindow(puttyHandle3, 0, 0, 600, 450, TRUE);
	
	puttyHandle4 = createPuttyWindow(g_appInstance, g_hWndMain, L"cmd");
	MoveWindow(puttyHandle4, 0, 0, 600, 450, TRUE);
	*/
	
	// 创建自定义同步滚动条
	g_hScrollTop = CreateWindowEx(
		0, _T("CustomScrollbar"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0, SCROLLBAR_WIDTH, 0,
		g_hWndMain, NULL, g_appInstance, NULL);

	g_hScrollBottom = CreateWindowEx(
		0, _T("CustomScrollbar"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0, SCROLLBAR_WIDTH, 0,
		g_hWndMain, NULL, g_appInstance, NULL);

	// 创建分隔条（置于顶层以确保可点击）
	g_hWndHSplit = CreateWindowEx(
		WS_EX_TOPMOST, _T("SplitterLineClass"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
		0, 0, 0, SPLITTER_SIZE,
		g_hWndMain, NULL, g_appInstance, NULL);

	g_hWndVSplitTop = CreateWindowEx(
		WS_EX_TOPMOST, _T("SplitterLineClass"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
		0, 0, SPLITTER_SIZE, 0,
		g_hWndMain, NULL, g_appInstance, NULL);

	g_hWndVSplitBottom = CreateWindowEx(
		WS_EX_TOPMOST, _T("SplitterLineClass"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
		0, 0, SPLITTER_SIZE, 0,
		g_hWndMain, NULL, g_appInstance, NULL);	
}

// 排列所有窗口位置
void ArrangeWindows() {
	GetClientRect(g_hWndMain, &g_rcMain);
	if (g_rcMain.right == 0 || g_rcMain.bottom == 0)
		return;

	int scrollPos = g_rcMain.right - SCROLLBAR_WIDTH;


	// 窗口1 - 上左
	if (puttyHandle1 && IsWindow(puttyHandle1)) {
		MoveWindow(puttyHandle1, 0, 0, g_nVSplitTopPos, g_nHSplitPos, TRUE);
	}

	// 顶部垂直分隔条
	MoveWindow(g_hWndVSplitTop, g_nVSplitTopPos, 0, SPLITTER_SIZE, g_nHSplitPos, TRUE);

	// 窗口2 - 上右
	if (puttyHandle2 && IsWindow(puttyHandle2)) {
		MoveWindow(puttyHandle2, g_nVSplitTopPos + SPLITTER_SIZE, 0, scrollPos - (g_nVSplitTopPos + SPLITTER_SIZE), g_nHSplitPos, TRUE);
	}
	// 水平分隔条
	MoveWindow(g_hWndHSplit, 0, g_nHSplitPos, g_rcMain.right, SPLITTER_SIZE, TRUE);
		
	// 窗口3 - 下左
	if (puttyHandle3 && IsWindow(puttyHandle3)) {
		MoveWindow(puttyHandle3, 0, g_nHSplitPos + SPLITTER_SIZE, g_nVSplitBottomPos, g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE), TRUE);
	}
	// 底部垂直分隔条
	MoveWindow(g_hWndVSplitBottom,
		g_nVSplitBottomPos, g_nHSplitPos + SPLITTER_SIZE,
		SPLITTER_SIZE, g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE),
		TRUE);

	// 窗口4 - 下右
	if (puttyHandle4 && IsWindow(puttyHandle4)) {
		MoveWindow(puttyHandle4,
			g_nVSplitBottomPos + SPLITTER_SIZE, g_nHSplitPos + SPLITTER_SIZE,
			scrollPos - (g_nVSplitBottomPos + SPLITTER_SIZE), g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE),
			TRUE);
	}

	LOG_DEBUG(L"ArrangeWindows: rc=%d, %d %d %d", g_rcMain.left, g_rcMain.top, g_rcMain.right, g_rcMain.bottom);
}

// 分隔条窗口过程
LRESULT CALLBACK SplitterProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// 转换坐标并转发鼠标消息到主窗口
	POINT pt;
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	ClientToScreen(hWnd, &pt);
	ScreenToClient(g_hWndMain, &pt);
	LPARAM convertedLParam = MAKELPARAM(pt.x, pt.y);

	switch (Msg)
	{
	case WM_LBUTTONDOWN:
	case WM_MOUSEMOVE:
	case WM_LBUTTONUP: {
		SendMessage(g_hWndMain, Msg, wParam, convertedLParam);
		return 0;
	}
	case WM_SETFOCUS:
		return 0;

	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
}


// 自定义滚动条窗口过程
LRESULT CALLBACK ScrollbarProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

// 检查点位于哪个分隔条上 - 增大检测区域，更容易选中
int GetSplitterAtPoint(POINT pt)
{
	// 检查水平分隔条 - 增大检测区域到10像素
	RECT rcHSplit;
	rcHSplit.top = g_nHSplitPos - 5;
	rcHSplit.bottom = g_nHSplitPos + 5;
	rcHSplit.left = 0;
	rcHSplit.right = g_rcMain.right;
	if (PtInRect(&rcHSplit, pt))
		return 1;

	// 检查顶部垂直分隔条
	RECT rcVSplitTop;
	rcVSplitTop.left = g_nVSplitTopPos - 5;
	rcVSplitTop.right = g_nVSplitTopPos + 5;
	rcVSplitTop.top = 0;
	rcVSplitTop.bottom = g_nHSplitPos;
	if (PtInRect(&rcVSplitTop, pt))
		return 2;

	// 检查底部垂直分隔条
	RECT rcVSplitBottom;
	rcVSplitBottom.left = g_nVSplitBottomPos - 5;
	rcVSplitBottom.right = g_nVSplitBottomPos + 5;
	rcVSplitBottom.top = g_nHSplitPos;
	rcVSplitBottom.bottom = g_rcMain.bottom;
	if (PtInRect(&rcVSplitBottom, pt))
		return 3;

	return 0;
}