#include "splitter.h"
#include "attach.h"
#include "logs.h"

static HINSTANCE g_appInstance;

HWND g_hWndMain;                // 主窗口句柄
HWND g_hWndHost;                // 空间容器层，解决putty等窗体移动残影问题
HWND g_hWndHSplit;              // 水平分隔条
HWND g_hWndVSplitTop;           // 顶部垂直分隔条
HWND g_hWndVSplitBottom;        // 底部垂直分隔条
HWND g_hScrollTop, g_hScrollBottom; // 同步滚动条
HWND puttyHandle1, puttyHandle2, puttyHandle3, puttyHandle4;// putty窗口

RECT g_rcMain;                  // 主窗口客户区矩形
int g_nHSplitPos = 450;         // 水平分隔条位置
int g_nVSplitTopPos = 600;      // 顶部垂直分隔条位置
int g_nVSplitBottomPos = 600;   // 底部垂直分隔条位置

// 分隔条位置比例
double g_dHSplitRatio = 0.5;     // 水平分隔条位置比例
double g_dVSplitTopRatio = 0.5;  // 顶部垂直分隔条位置比例
double g_dVSplitBottomRatio = 0.5; // 底部垂直分隔条位置比例

const int SPLITTER_SIZE = 10;    // 分隔条粗细
const int SCROLLBAR_WIDTH = 20;  // 加宽滚动条便于可见
const int THUMB_MIN_HEIGHT = 10; // 加大滑块便于点击
const int SCROLL_MARGIN = 2;     // 滚动条内边距

// 确保窗口类只注册一次（首次调用时注册）
static bool g_splitMainClassRegistered = false;
static bool g_splitLineClassRegistered = false;
static bool g_scrollbarClassRegistered = false;

int g_nDragging = 0;            // 0=未拖动, 1=水平, 2=顶部垂直, 3=底部垂直, 4=顶部滚动条, 5=底部滚动条
#define TIMER_ID_RESIZE 11      //定时器

// 全局变量
HWINEVENTHOOK g_hMoveSizeHook = NULL;  // 监听移动的钩子
HWINEVENTHOOK g_hMoveEndHook = NULL;   // 移动结束的钩子
HWND g_hDraggingPuTTY = NULL;          // putty窗口拖动状态
int g_nStartRegion = 0;                // 窗口移动初始位置

// 滚动相关变量（新增滑块状态和拖动信息）
int g_nScrollPosTop = 0;        // 顶部滚动条位置
int g_nScrollPosBottom = 0;     // 底部滚动条位置
int g_nContentHeightTop = 10000; // 顶部内容高度
int g_nContentHeightBottom = 10000; // 底部内容高度
int g_nVisibleHeightTop = 0;    // 顶部可见高度
int g_nVisibleHeightBottom = 0; // 底部可见高度
int g_nScrollRangeTop = 0;      // 顶部可滚动范围
int g_nScrollRangeBottom = 0;   // 底部可滚动范围

// 滚动条状态枚举（新增）
typedef enum {
	SB_STATE_NORMAL,   // 正常
	SB_STATE_HOVER,    // 悬停
	SB_STATE_DRAGGING  // 拖动中
} ScrollBarState;

// 滚动条私有数据（存储每个滚动条的状态和资源，支持多个滚动条）
typedef struct {
	ScrollBarState thumbState;  // 滑块状态
	BOOL isDragging;            // 是否正在拖动
	int dragYOffset;            // 拖动时鼠标在滑块内的Y偏移
	// 绘图资源（避免重复创建销毁）
	HBRUSH hTrackBrush;         // 轨道画刷（浅灰）
	HBRUSH hThumbNormalBrush;   // 滑块正常画刷（亮蓝）
	HBRUSH hThumbHoverBrush;    // 滑块悬停画刷（深蓝）
	HBRUSH hBorderBrush;        // 边框画刷（黑色）
} ScrollBarData;
static ScrollBarData g_scrollTopData; // 顶部滚动条数据
static ScrollBarData g_scrollBottomData; // 底部滚动条数据

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
	// 注册窗口类，内部判断只会注册一次
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

	// 初始化PuTTY拖动监听钩子
	StartPuTTYHooks();
	return g_hWndHost;
}

// 计算滑块高度（根据滚动范围和可见高度比例）
static int CalcThumbHeight(int scrollRange, int visibleHeight, int clientHeight) {
	if (scrollRange <= 0) return clientHeight; // 无滚动时滑块占满轨道
	int thumbH = (int)((double)visibleHeight / (visibleHeight + scrollRange) * clientHeight);
	return max(thumbH, THUMB_MIN_HEIGHT);
}

// 计算滑块Y坐标（根据当前滚动位置）
static int CalcThumbY(int scrollPos, int scrollRange, int thumbHeight, int clientHeight) {
	if (scrollRange <= 0) return 0; // 无滚动时滑块置顶
	int trackAvailH = clientHeight - thumbHeight; // 轨道可用高度
	return (int)((double)scrollPos / scrollRange * trackAvailH);
}

// 从鼠标位置计算滚动位置（拖动时用）
static int CalcPosFromMouse(int mouseY, int dragOffset, int scrollRange, int thumbHeight, int clientHeight) {
	if (scrollRange <= 0) return 0;
	int trackAvailH = clientHeight - thumbHeight;
	if (trackAvailH <= 0) return 0;
	// 鼠标在轨道中的相对Y坐标 = 鼠标Y - 拖动偏移 - 客户区顶部
	int relY = mouseY - dragOffset;
	// 限制在轨道范围内
	relY = max(0, min(relY, trackAvailH));
	return (int)((double)relY / trackAvailH * scrollRange);
}

// 初始化滚动条资源（避免重复创建）
static void InitScrollBarData(ScrollBarData* sbData) {
	if (sbData->hTrackBrush) return; // 已初始化过

	// 初始化绘图资源（创建画笔/画刷）
	sbData->hTrackBrush = CreateSolidBrush(RGB(230, 230, 230));       // 轨道：浅灰
	sbData->hThumbNormalBrush = CreateSolidBrush(RGB(100, 180, 255)); // 滑块正常：亮蓝
	sbData->hThumbHoverBrush = CreateSolidBrush(RGB(50, 150, 255));   // 滑块悬停：深蓝
	sbData->hBorderBrush = CreateSolidBrush(RGB(0, 0, 0));            // 边框：黑色
	// 初始化状态变量
	sbData->thumbState = SB_STATE_NORMAL;
	sbData->isDragging = FALSE;
	sbData->dragYOffset = 0;
}

// 释放滚动条资源（防止内存泄漏）
static void FreeScrollBarData(ScrollBarData* sbData) {
	if (sbData->hTrackBrush) DeleteObject(sbData->hTrackBrush);
	if (sbData->hThumbNormalBrush) DeleteObject(sbData->hThumbNormalBrush);
	if (sbData->hThumbHoverBrush) DeleteObject(sbData->hThumbHoverBrush);
	if (sbData->hBorderBrush) DeleteObject(sbData->hBorderBrush);
	ZeroMemory(sbData, sizeof(ScrollBarData));
}

// 更新滚动范围
void UpdateScrollRanges() {
	// 计算顶部区域滚动范围
	g_nVisibleHeightTop = g_nHSplitPos;
	g_nScrollRangeTop = max(0, g_nContentHeightTop - g_nVisibleHeightTop);

	// 计算底部区域滚动范围
	g_nVisibleHeightBottom = g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE);
	g_nScrollRangeBottom = max(0, g_nContentHeightBottom - g_nVisibleHeightBottom);

	// 确保滚动位置在有效范围内
	g_nScrollPosTop = min(g_nScrollPosTop, g_nScrollRangeTop);
	g_nScrollPosBottom = min(g_nScrollPosBottom, g_nScrollRangeBottom);

	// 滚动条位置变化后强制重绘
	if (g_hScrollTop) InvalidateRect(g_hScrollTop, NULL, FALSE);
	if (g_hScrollBottom) InvalidateRect(g_hScrollBottom, NULL, FALSE);

	LOG_DEBUG(L"split scroll pos g_nScrollPosTop", g_nScrollPosTop);
}

// 向PuTTY窗口发送滚动消息（与deltaY关联，调整滚动速度）
void SendScrollMessageToPuTTY(HWND hWnd, int deltaY) {

	if (!hWnd || !IsWindow(hWnd)) return;

	// 计算滚动强度（根据deltaY绝对值确定滚动次数）
	// deltaY绝对值越大，滚动次数越多（速度越快）
	int scrollAmount = abs(deltaY);
	// 每次基础滚动对应的delta值（可根据需要调整灵敏度）
	const int BASE_DELTA = 8;
	// 计算滚动次数（至少1次，最多10次避免滚动过快）
	int scrollCount = max(1, min(scrollAmount / BASE_DELTA, 10));

	// 确定滚动方向（上滚/下滚）
	BOOL isUp = deltaY > 0;
	WPARAM wheelParam = isUp ? (-WHEEL_DELTA << 16) : (WHEEL_DELTA << 16);
	WORD vkCode = isUp ? VK_UP : VK_DOWN;

	LOG_DEBUG(L"split send to hwnd %p %d %d", hWnd, deltaY, scrollCount);
	// 根据deltaY大小发送多次滚动消息
	for (int i = 0; i < scrollCount; i++) {
		// 发送鼠标滚轮消息
		SendMessage(hWnd, WM_MOUSEWHEEL, wheelParam, MAKELPARAM(10, 10));

		// 发送键盘滚动消息作为备份
		//SendMessage(hWnd, WM_KEYDOWN, vkCode, 0);
		// 轻微延迟确保按键被正确识别（可根据需要调整）
		//Sleep(5);
		//SendMessage(hWnd, WM_KEYUP, vkCode, 0);
	}
}


// 同步滚动处理
void SyncScroll(int pos, bool isTop) {
	int oldPos = isTop ? g_nScrollPosTop : g_nScrollPosBottom;
	int scrollRange = isTop ? g_nScrollRangeTop : g_nScrollRangeBottom;

	// 限制滚动位置在有效范围内
	pos = max(0, min(pos, scrollRange));
	if (pos == oldPos) return; // 没有变化则返回
	int deltaY = pos - oldPos; // 计算滚动变化量

	// 移动对应区域的PuTTY窗口并发送滚动消息
	if (isTop) {
		g_nScrollPosTop = pos;
		if (puttyHandle1 && IsWindow(puttyHandle1)) {
			SendScrollMessageToPuTTY(puttyHandle1, deltaY);
		}
		if (puttyHandle2 && IsWindow(puttyHandle2)) {
			SendScrollMessageToPuTTY(puttyHandle2, deltaY);
		}
		// 同步顶部滚动条重绘
		if (g_hScrollTop) InvalidateRect(g_hScrollTop, NULL, FALSE);
	}
	else {
		g_nScrollPosBottom = pos;
		if (puttyHandle3 && IsWindow(puttyHandle3)) {
			SendScrollMessageToPuTTY(puttyHandle3, deltaY);
		}
		if (puttyHandle4 && IsWindow(puttyHandle4)) {
			SendScrollMessageToPuTTY(puttyHandle4, deltaY);
		}
		// 同步底部滚动条重绘
		if (g_hScrollBottom) InvalidateRect(g_hScrollBottom, NULL, FALSE);
	}
}

// 分屏窗口过程
LRESULT CALLBACK SplitWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	if (msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_MOUSEWHEEL)
	{
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		if (msg == WM_MOUSEWHEEL) {
			// 转换滚轮消息的坐标
			ScreenToClient(hWnd, &pt);
		}
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
		// 初始化滚动条数据（创建绘图资源）
		InitScrollBarData(&g_scrollTopData);
		InitScrollBarData(&g_scrollBottomData);
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
		MoveWindow(g_hWndHost, g_rcMain.left, g_rcMain.top, g_rcMain.right - g_rcMain.left, g_rcMain.bottom - g_rcMain.top, TRUE);
		ArrangeWindows();
		UpdateScrollRanges();
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
			UpdateScrollRanges();
		}
		return 0;
	}
	case WM_LBUTTONDOWN:
		// 开始拖动
		g_nDragging = GetSplitterAtPoint(pt);
		if (g_nDragging > 0 && g_nDragging <= 3)
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
		if (g_nDragging > 0 && g_nDragging <= 3)
		{
			g_nDragging = 0;
			ReleaseCapture();
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			return 0;
		}
		break;
	case WM_MOUSEWHEEL: {
		// 下滚为负，上滚为正
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

		// 判断鼠标在哪个区域
		bool isTopRegion = (pt.y < g_nHSplitPos);

		int visibleHeight = isTopRegion ? g_nVisibleHeightTop : g_nVisibleHeightBottom;
		int lineStep = visibleHeight / 20;
		if (zDelta > 0) {
			lineStep = -lineStep;
		}

		int newPos = isTopRegion ?
			(g_nScrollPosTop + lineStep) :
			(g_nScrollPosBottom + lineStep);

		newPos = max(0, newPos);
		LOG_DEBUG(L"split SplitWindowProc MOUSEWHEEL  %d, pos %d, newpos %d", zDelta, isTopRegion ? g_nScrollPosTop : g_nScrollPosBottom, newPos);
		SyncScroll(newPos, isTopRegion);
		return 0;
	}
	case WM_DESTROY: {
		g_hWndMain = NULL;
		g_hWndHost = NULL;
		// 重置比例
		g_dHSplitRatio = 0.5;
		g_dVSplitTopRatio = 0.5;
		g_dVSplitBottomRatio = 0.5;
		// 重置putty句柄
		puttyHandle1 = NULL;
		puttyHandle2 = NULL;
		puttyHandle3 = NULL;
		puttyHandle4 = NULL;
		// 卸载拖动监听钩子
		StopPuTTYHooks();
		// 释放滚动条资源
		FreeScrollBarData(&g_scrollTopData);
		FreeScrollBarData(&g_scrollBottomData);
		return 0;
	}
	}

	return DefWindowProcW(hWnd, msg, wParam, lParam);
}


// 嵌入PuTTY窗口到主窗口
void insertSplitWindow(HWND puttyHwnd, int pos) {
	if (!puttyHwnd || !IsWindow(puttyHwnd)) return;

	// 保存旧句柄用于清理
	HWND oldHandle = NULL;
	switch (pos) {
	case 1: oldHandle = puttyHandle1; puttyHandle1 = puttyHwnd; break;
	case 2: oldHandle = puttyHandle2; puttyHandle2 = puttyHwnd; break;
	case 3: oldHandle = puttyHandle3; puttyHandle3 = puttyHwnd; break;
	case 4: oldHandle = puttyHandle4; puttyHandle4 = puttyHwnd; break;
	default: oldHandle = puttyHandle1; puttyHandle1 = puttyHwnd; break;
	}

	// 解除旧窗口的父窗口关联
	if (oldHandle && IsWindow(oldHandle)) {
		SetParent(oldHandle, NULL);
	}

	ArrangeWindows();
}

// 创建子窗口
void CreateChildWindows(HWND hWnd)
{
	g_hWndMain = hWnd;

	// 创建宿主窗口
	g_hWndHost = CreateWindowExW(
		0,
		L"Static",
		L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0,
		1200, 900,
		g_hWndMain,
		NULL,
		g_appInstance,
		NULL
	);

	// 创建分隔条（置于顶层以确保可点击）
	g_hWndHSplit = CreateWindowEx(
		WS_EX_TOPMOST, _T("SplitterLineClass"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
		0, 0, 0, SPLITTER_SIZE,
		g_hWndHost, NULL, g_appInstance, NULL);

	g_hWndVSplitTop = CreateWindowEx(
		WS_EX_TOPMOST, _T("SplitterLineClass"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
		0, 0, SPLITTER_SIZE, 0,
		g_hWndHost, NULL, g_appInstance, NULL);

	g_hWndVSplitBottom = CreateWindowEx(
		WS_EX_TOPMOST, _T("SplitterLineClass"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
		0, 0, SPLITTER_SIZE, 0,
		g_hWndHost, NULL, g_appInstance, NULL);

	// 创建自定义同步滚动条（添加WS_BORDER确保可见）
	g_hScrollTop = CreateWindowEx(
		0, _T("CustomScrollbar"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
		0, 0, SCROLLBAR_WIDTH, 0,
		g_hWndHost, NULL, g_appInstance, NULL);

	g_hScrollBottom = CreateWindowEx(
		0, _T("CustomScrollbar"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
		0, 0, SCROLLBAR_WIDTH, 0,
		g_hWndHost, NULL, g_appInstance, NULL);

	// 检查滚动条是否创建成功
	if (!g_hScrollTop || !g_hScrollBottom) {
		MessageBoxW(NULL, L"滚动条创建失败", GetString(IDS_MESSAGE_CAPTION), MB_OK | MB_ICONERROR);
	}
}

// 辅助函数：计算两个矩形的重叠面积
int CalculateOverlapArea(const RECT* rc1, const RECT* rc2) {
	int left = max(rc1->left, rc2->left);
	int right = min(rc1->right, rc2->right);
	int top = max(rc1->top, rc2->top);
	int bottom = min(rc1->bottom, rc2->bottom);

	// 无重叠
	if (left >= right || top >= bottom) {
		return 0;
	}

	// 重叠面积 = 宽 * 高
	return (right - left) * (bottom - top);
}

// 优化后的区域判断：窗口50%以上面积在该区域则判定归属
int GetWindowRegion(HWND hWnd) {
	if (!hWnd || !IsWindow(hWnd) || !g_hWndMain) return 0;

	RECT rcWnd, rcMain;
	GetWindowRect(hWnd, &rcWnd);
	ScreenToClient(g_hWndMain, (LPPOINT)&rcWnd);  // 转主窗口客户区坐标
	ScreenToClient(g_hWndMain, ((LPPOINT)&rcWnd) + 1);
	GetClientRect(g_hWndMain, &rcMain);
	int scrollPos = rcMain.right - SCROLLBAR_WIDTH;

	// 计算窗口总面积
	int wndWidth = rcWnd.right - rcWnd.left;
	int wndHeight = rcWnd.bottom - rcWnd.top;
	int totalArea = wndWidth * wndHeight;
	if (totalArea == 0) return 0;  // 无效窗口

	// 定义4个目标区域
	RECT rcRegion1 = { 0, 0, g_nVSplitTopPos, g_nHSplitPos };
	RECT rcRegion2 = { g_nVSplitTopPos + SPLITTER_SIZE, 0, scrollPos, g_nHSplitPos };
	RECT rcRegion3 = { 0, g_nHSplitPos + SPLITTER_SIZE, g_nVSplitBottomPos, rcMain.bottom };
	RECT rcRegion4 = { g_nVSplitBottomPos + SPLITTER_SIZE, g_nHSplitPos + SPLITTER_SIZE, scrollPos, rcMain.bottom };

	// 计算与每个区域的重叠面积
	int area1 = CalculateOverlapArea(&rcWnd, &rcRegion1);
	int area2 = CalculateOverlapArea(&rcWnd, &rcRegion2);
	int area3 = CalculateOverlapArea(&rcWnd, &rcRegion3);
	int area4 = CalculateOverlapArea(&rcWnd, &rcRegion4);

	// 如果完全覆盖某一区域达90%
	if (area1 > 0.9 * (rcRegion1.right - rcRegion1.left) * (rcRegion1.bottom - rcRegion1.top)) {
		return 1;
	}
	else if (area2 > 0.9 * (rcRegion2.right - rcRegion2.left) * (rcRegion2.bottom - rcRegion2.top)) {
		return 2;
	}
	else if (area3 > 0.9 * (rcRegion3.right - rcRegion3.left) * (rcRegion3.bottom - rcRegion3.top)) {
		return 3;
	}
	else if (area4 > 0.9 * (rcRegion4.right - rcRegion4.left) * (rcRegion4.bottom - rcRegion4.top)) {
		return 4;
	}

	// 找到最大重叠区域
	int maxArea = max(max(area1, area2), max(area3, area4));

	// 只有当最大重叠面积超过窗口总面积的50%时才判定归属
	if (maxArea > totalArea / 2) {
		if (maxArea == area1) return 1;
		if (maxArea == area2) return 2;
		if (maxArea == area3) return 3;
		if (maxArea == area4) return 4;
	}

	return 0;  // 未满足50%重叠条件
}

// 辅助函数：根据区域获取对应PuTTY句柄
HWND GetHandleByRegion(int region) {
	switch (region) {
	case 1: return puttyHandle1;
	case 2: return puttyHandle2;
	case 3: return puttyHandle3;
	case 4: return puttyHandle4;
	default: return NULL;
	}
}

// 辅助函数：设置区域对应的PuTTY句柄
void SetHandleByRegion(int region, HWND hWnd) {
	// 如果是无效的句柄，清掉
	if (hWnd && !IsWindow(hWnd)) {
		hWnd = NULL;
	}
	switch (region) {
	case 1: puttyHandle1 = hWnd; break;
	case 2: puttyHandle2 = hWnd; break;
	case 3: puttyHandle3 = hWnd; break;
	case 4: puttyHandle4 = hWnd; break;
	}
}

// 辅助函数：交换两个区域的PuTTY句柄
void SwapRegionHandles(int regionA, int regionB) {
	HWND hA = GetHandleByRegion(regionA);
	HWND hB = GetHandleByRegion(regionB);
	SetHandleByRegion(regionA, hB);
	SetHandleByRegion(regionB, hA);
	LOG_DEBUG(L"Swap PuTTY: Region%d <-> Region%d", regionA, regionB);
}


// 判断窗口是否为我们管理的PuTTY窗口
BOOL IsManagedPuTTY(HWND hWnd) {
	return hWnd == puttyHandle1 || hWnd == puttyHandle2 ||
		hWnd == puttyHandle3 || hWnd == puttyHandle4;
}

// 钩子回调函数（只关注移动/调整大小结束事件）
void CALLBACK MoveSizeChangeHookProc(
	HWINEVENTHOOK hHook, DWORD event,
	HWND hWnd, LONG idObject, LONG idChild,
	DWORD idEventThread, DWORD dwmsEventTime
) {
	// 只处理窗口本身且是我们的PuTTY窗口
	if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF)
		return;
	if (!IsManagedPuTTY(hWnd))
		return;

	if (event == EVENT_SYSTEM_MOVESIZESTART) {
		if (!g_hDraggingPuTTY) { // 首次触发视为拖动开始
			g_hDraggingPuTTY = hWnd;
			g_nStartRegion = GetWindowRegion(hWnd); // 记录初始区域
			LOG_DEBUG(L"move start %d %p", g_nStartRegion, hWnd);
		}
		return;
	}
	if (event == EVENT_SYSTEM_MOVESIZEEND) {
		if (g_hDraggingPuTTY == hWnd) {
			int endRegion = GetWindowRegion(hWnd);
			if (endRegion != 0 && endRegion != g_nStartRegion) {
				// 执行句柄交换逻辑
				HWND hTarget = GetHandleByRegion(endRegion);
				if (hTarget) {
					SwapRegionHandles(g_nStartRegion, endRegion);
				}
				else {
					SetHandleByRegion(endRegion, hWnd);
					SetHandleByRegion(g_nStartRegion, NULL);
				}
				ArrangeWindows();
			}
			LOG_DEBUG(L"move end %d %d %p", g_nStartRegion, endRegion, hWnd);
			g_hDraggingPuTTY = NULL;  // 重置拖动状态
			g_nStartRegion = 0;
		}
	}
}

// 启动钩子（同时监听位置变化和移动结束）
void StartPuTTYHooks() {
	StopPuTTYHooks();

	// 2个钩子不能合成一个，会收不到消息
	// 注册位置变化钩子
	g_hMoveSizeHook = SetWinEventHook(
		EVENT_SYSTEM_MOVESIZESTART, EVENT_SYSTEM_MOVESIZESTART,  // 窗口位置变化（拖动开始）
		NULL, MoveSizeChangeHookProc,                            // 回调函数
		0, 0,                                                    // 所有进程、所有线程
		WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS          // 跨进程监听，非上下文钩子
	);

	// 注册移动结束钩子
	g_hMoveEndHook = SetWinEventHook(
		EVENT_SYSTEM_MOVESIZEEND, EVENT_SYSTEM_MOVESIZEEND, // 窗口移动结束（拖动结束）
		NULL, MoveSizeChangeHookProc,
		0, 0,
		WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
	);
}

// 停止钩子
void StopPuTTYHooks() {
	if (g_hMoveSizeHook) {
		UnhookWinEvent(g_hMoveSizeHook);
		g_hMoveSizeHook = NULL;
	}
	if (g_hMoveEndHook) {
		UnhookWinEvent(g_hMoveEndHook);
		g_hMoveEndHook = NULL;
	}
}

// 排列窗口
void ArrangeWindows() {
	GetClientRect(g_hWndMain, &g_rcMain);
	if (g_rcMain.right == 0 || g_rcMain.bottom == 0)
		return;
	LOG_DEBUG(L"Arrange %p %p %p %p", puttyHandle1, puttyHandle2, puttyHandle3, puttyHandle4);
	int scrollPos = g_rcMain.right - SCROLLBAR_WIDTH;

	// 调整所有窗口位置（保持原有逻辑）
	// 窗口1 - 上左
	if (puttyHandle1 && IsWindow(puttyHandle1)) {
		// 增加一个间隙，方便定位拖动条
		MoveWindow(puttyHandle1, 0, 0, g_nVSplitTopPos-1, g_nHSplitPos, TRUE);
	}

	// 顶部垂直分隔条
	MoveWindow(g_hWndVSplitTop, g_nVSplitTopPos, 0, SPLITTER_SIZE, g_nHSplitPos, TRUE);

	// 窗口2 - 上右
	if (puttyHandle2 && IsWindow(puttyHandle2)) {
		MoveWindow(puttyHandle2, g_nVSplitTopPos + SPLITTER_SIZE, 0,
			scrollPos - (g_nVSplitTopPos + SPLITTER_SIZE), g_nHSplitPos, TRUE);
	}

	// 水平分隔条
	MoveWindow(g_hWndHSplit, 0, g_nHSplitPos, g_rcMain.right, SPLITTER_SIZE, TRUE);

	// 窗口3 - 下左
	if (puttyHandle3 && IsWindow(puttyHandle3)) {
		// 增加一个间隙，方便定位拖动条
		MoveWindow(puttyHandle3, 0, g_nHSplitPos + SPLITTER_SIZE,
			g_nVSplitBottomPos - 1, g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE), TRUE);
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
			scrollPos - (g_nVSplitBottomPos + SPLITTER_SIZE),
			g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE),
			TRUE);
	}

	// 调整滚动条位置（确保可见）
	if (g_hScrollTop) {
		MoveWindow(g_hScrollTop, scrollPos, 0, SCROLLBAR_WIDTH, g_nHSplitPos, TRUE);
		ShowWindow(g_hScrollTop, SW_SHOW); // 强制显示
	}
	if (g_hScrollBottom) {
		MoveWindow(g_hScrollBottom, scrollPos, g_nHSplitPos + SPLITTER_SIZE,
			SCROLLBAR_WIDTH, g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE), TRUE);
		ShowWindow(g_hScrollBottom, SW_SHOW); // 强制显示
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

// 自定义滚动条窗口过程（修复版）
LRESULT CALLBACK ScrollbarProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	bool isTop = (hWnd == g_hScrollTop);
	ScrollBarData* sbData = isTop ? &g_scrollTopData : &g_scrollBottomData;
	int* pPos = isTop ? &g_nScrollPosTop : &g_nScrollPosBottom;
	int scrollRange = isTop ? g_nScrollRangeTop : g_nScrollRangeBottom;
	int visibleHeight = isTop ? g_nVisibleHeightTop : g_nVisibleHeightBottom;

	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	int clientHeight = clientRect.bottom - clientRect.top;
	int thumbHeight = CalcThumbHeight(scrollRange, visibleHeight, clientHeight);
	int thumbY = CalcThumbY(*pPos, scrollRange, thumbHeight, clientHeight);

	// 处理鼠标位置
	POINT pt;
	if (Msg == WM_MOUSEMOVE || Msg == WM_LBUTTONDOWN || Msg == WM_LBUTTONUP) {
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
	}

	switch (Msg) {
	case WM_PAINT: {
		LOG_DEBUG(L"splitter ScrollbarProc WM_PAINT");
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		// 创建内存DC用于双缓冲
		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBitmap = CreateCompatibleBitmap(hdc,
			clientRect.right - clientRect.left,
			clientRect.bottom - clientRect.top);
		HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap);

		HGDIOBJ oldBrush;

		// 1. 绘制滚动条背景
		HBRUSH bgBrush = CreateSolidBrush(RGB(255, 240, 240));
		oldBrush = SelectObject(memDC, bgBrush);
		Rectangle(memDC, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
		SelectObject(memDC, oldBrush);
		DeleteObject(bgBrush);

		// 2. 绘制轨道
		RECT trackRect = {
			clientRect.left + SCROLL_MARGIN,
			clientRect.top + SCROLL_MARGIN,
			clientRect.right - SCROLL_MARGIN,
			clientRect.bottom - SCROLL_MARGIN
		};
		oldBrush = SelectObject(memDC, sbData->hTrackBrush);
		Rectangle(memDC, trackRect.left, trackRect.top, trackRect.right, trackRect.bottom);
		SelectObject(memDC, oldBrush);

		// 3. 绘制滑块
		RECT thumbRect = {
			trackRect.left,
			trackRect.top + thumbY,
			trackRect.right,
			trackRect.top + thumbY + thumbHeight
		};

		HBRUSH thumbBrush = (sbData->thumbState == SB_STATE_HOVER || sbData->thumbState == SB_STATE_DRAGGING)
			? sbData->hThumbHoverBrush : sbData->hThumbNormalBrush;

		oldBrush = SelectObject(memDC, thumbBrush);
		Rectangle(memDC, thumbRect.left, thumbRect.top, thumbRect.right, thumbRect.bottom);
		SelectObject(memDC, oldBrush);

		// 4. 绘制滑块边框
		HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
		HPEN oldPen = (HPEN)SelectObject(memDC, borderPen);
		MoveToEx(memDC, thumbRect.left, thumbRect.top, NULL);
		LineTo(memDC, thumbRect.right, thumbRect.top);
		LineTo(memDC, thumbRect.right, thumbRect.bottom);
		LineTo(memDC, thumbRect.left, thumbRect.bottom);
		LineTo(memDC, thumbRect.left, thumbRect.top);
		SelectObject(memDC, oldPen);
		DeleteObject(borderPen);

		// 将内存DC内容一次性复制到屏幕DC
		BitBlt(hdc, clientRect.left, clientRect.top,
			clientRect.right - clientRect.left,
			clientRect.bottom - clientRect.top,
			memDC, 0, 0, SRCCOPY);

		// 清理资源
		SelectObject(memDC, oldBitmap);
		DeleteObject(memBitmap);
		DeleteDC(memDC);
		EndPaint(hWnd, &ps);
		return 0;
	}

	case WM_ERASEBKGND:
		// 禁止系统擦除背景，减少闪烁
		return TRUE;

	case WM_MOUSEMOVE: {
		LOG_DEBUG(L"splitter ScrollbarProc WM_MOUSEMOVE");
		RECT thumbRect = {
			clientRect.left + SCROLL_MARGIN,
			clientRect.top + SCROLL_MARGIN + thumbY,
			clientRect.right - SCROLL_MARGIN,
			clientRect.top + SCROLL_MARGIN + thumbY + thumbHeight
		};

		if (sbData->isDragging) {
			int newPos = CalcPosFromMouse(pt.y, sbData->dragYOffset, scrollRange, thumbHeight, clientHeight);
			// 只在位置变化时才重绘
			if (newPos != *pPos) {
				SyncScroll(newPos, isTop);
			}
			return 0;
		}

		ScrollBarState newState = PtInRect(&thumbRect, pt) ? SB_STATE_HOVER : SB_STATE_NORMAL;
		if (newState != sbData->thumbState) {
			sbData->thumbState = newState;
			// 只重绘滑块区域而非整个控件
			InvalidateRect(hWnd, &thumbRect, FALSE);
			SetCursor(LoadCursor(NULL, IDC_HAND));
		}
		return 0;
	}

	case WM_LBUTTONDOWN: {
		LOG_DEBUG(L"splitter ScrollbarProc WM_LBUTTONDOWN");
		// 滑块区域
		RECT thumbRect = {
			clientRect.left + SCROLL_MARGIN,
			clientRect.top + SCROLL_MARGIN + thumbY,
			clientRect.right - SCROLL_MARGIN,
			clientRect.top + SCROLL_MARGIN + thumbY + thumbHeight
		};

		if (PtInRect(&thumbRect, pt)) {
			// 开始拖动滑块
			sbData->isDragging = TRUE;
			sbData->thumbState = SB_STATE_DRAGGING;
			sbData->dragYOffset = pt.y - thumbRect.top; // 记录鼠标在滑块内的偏移
			SetCapture(hWnd);
			InvalidateRect(hWnd, NULL, FALSE);
			return 0;
		}

		// 点击轨道空白处
		int lineStep = visibleHeight / 2; // 半页滚动
		int newPos = *pPos;

		if (pt.y < thumbY + SCROLL_MARGIN) {
			newPos -= lineStep; // 上滚
		}
		else {
			newPos += lineStep; // 下滚
		}

		SyncScroll(newPos, isTop);
		return 0;
	}

	case WM_LBUTTONUP:
	case WM_CAPTURECHANGED: {
		if (sbData->isDragging) {
			sbData->isDragging = FALSE;
			sbData->thumbState = SB_STATE_NORMAL;
			ReleaseCapture();
			InvalidateRect(hWnd, NULL, FALSE);
		}
		return 0;
	}

	case WM_VSCROLL: {
		LOG_DEBUG(L"splitter ScrollbarProc WM_VSCROLL");
		int scrollCode = LOWORD(wParam);
		int newPos = *pPos;
		int lineStep = visibleHeight / 20;  // 行滚动
		int pageStep = visibleHeight;       // 页滚动

		switch (scrollCode) {
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			newPos = HIWORD(wParam);
			break;
		case SB_LINEUP:
			newPos -= lineStep;
			break;
		case SB_LINEDOWN:
			newPos += lineStep;
			break;
		case SB_PAGEUP:
			newPos -= pageStep;
			break;
		case SB_PAGEDOWN:
			newPos += pageStep;
			break;
		case SB_TOP:
			newPos = 0;
			break;
		case SB_BOTTOM:
			newPos = scrollRange;
			break;
		default:
			return 0;
		}

		SyncScroll(newPos, isTop);
		return 0;
	}

	case WM_SETCURSOR:
		// 始终显示手型光标提示可交互
		SetCursor(LoadCursor(NULL, IDC_HAND));
		return 1;

	case WM_DESTROY:
		// 释放资源
		FreeScrollBarData(sbData);
		break;
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

// 检查鼠标点位于哪个分隔条上 - 增大检测区域，更容易选中
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
