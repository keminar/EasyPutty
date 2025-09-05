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
HWND puttyHandle1, puttyHandle2, puttyHandle3, puttyHandle4;

RECT g_rcMain;                  // 主窗口客户区矩形
int g_nHSplitPos = 450;         // 水平分隔条位置
int g_nVSplitTopPos = 600;      // 顶部垂直分隔条位置
int g_nVSplitBottomPos = 600;   // 底部垂直分隔条位置

// 分隔条位置比例
double g_dHSplitRatio = 0.5;     // 水平分隔条位置比例
double g_dVSplitTopRatio = 0.5;  // 顶部垂直分隔条位置比例
double g_dVSplitBottomRatio = 0.5; // 底部垂直分隔条位置比例

const int SPLITTER_SIZE = 10;    // 分隔条粗细
const int SCROLLBAR_WIDTH = 20; // 滚动条宽度

// 确保窗口类只注册一次（首次调用时注册）
static bool g_splitMainClassRegistered = false;
static bool g_splitLineClassRegistered = false;
static bool g_scrollbarClassRegistered = false;

int g_nDragging = 0;            // 0=未拖动, 1=水平, 2=顶部垂直, 3=底部垂直, 4=顶部滚动条, 5=底部滚动条
#define TIMER_ID_RESIZE 11      //定时器

// 全局变量
HWINEVENTHOOK g_hMoveSizeHook = NULL;  // 专门监听移动/调整大小结束的钩子
HWINEVENTHOOK g_hMoveEndHook = NULL;
HWND g_hDraggingPuTTY = NULL;
int g_nStartRegion = 0;

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

	// 初始化PuTTY拖动监听钩子
	StartPuTTYHooks();
	return g_hWndHost;
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
		MoveWindow(g_hWndHost, g_rcMain.left, g_rcMain.top, g_rcMain.right - g_rcMain.left, g_rcMain.bottom - g_rcMain.top, TRUE);
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

	// 创建自定义同步滚动条
	g_hScrollTop = CreateWindowEx(
		0, _T("CustomScrollbar"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0, SCROLLBAR_WIDTH, 0,
		g_hWndHost, NULL, g_appInstance, NULL);

	g_hScrollBottom = CreateWindowEx(
		0, _T("CustomScrollbar"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0, SCROLLBAR_WIDTH, 0,
		g_hWndHost, NULL, g_appInstance, NULL);

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
	} else if (area2 > 0.9 * (rcRegion2.right - rcRegion2.left) * (rcRegion2.bottom - rcRegion2.top)) {
		return 2;
	}else if (area3 > 0.9 * (rcRegion3.right - rcRegion3.left) * (rcRegion3.bottom - rcRegion3.top)) {
		return 3;
	}else if (area3 > 0.9 * (rcRegion4.right - rcRegion4.left) * (rcRegion4.bottom - rcRegion4.top)) {
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

// 2. 辅助函数：根据区域获取对应PuTTY句柄
HWND GetHandleByRegion(int region) {
	switch (region) {
	case 1: return puttyHandle1;
	case 2: return puttyHandle2;
	case 3: return puttyHandle3;
	case 4: return puttyHandle4;
	default: return NULL;
	}
}

// 3. 辅助函数：设置区域对应的PuTTY句柄
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

// 4. 辅助函数：交换两个区域的PuTTY句柄
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
		if (g_hDraggingPuTTY == hWnd){
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
		MoveWindow(puttyHandle1, 0, 0, g_nVSplitTopPos, g_nHSplitPos, TRUE);
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
		MoveWindow(puttyHandle3, 0, g_nHSplitPos + SPLITTER_SIZE,
			g_nVSplitBottomPos, g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE), TRUE);
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