#include "splitter.h"
#include "attach.h"
#include "logs.h"

static HINSTANCE g_appInstance;

HWND g_hWndMain;                // �����ھ��
HWND g_hWndHost;                // �ռ������㣬���putty�ȴ����ƶ���Ӱ����
HWND g_hWndHSplit;              // ˮƽ�ָ���
HWND g_hWndVSplitTop;           // ������ֱ�ָ���
HWND g_hWndVSplitBottom;        // �ײ���ֱ�ָ���
HWND g_hScrollTop, g_hScrollBottom; // ͬ��������
HWND puttyHandle1, puttyHandle2, puttyHandle3, puttyHandle4;// putty����

RECT g_rcMain;                  // �����ڿͻ�������
int g_nHSplitPos = 450;         // ˮƽ�ָ���λ��
int g_nVSplitTopPos = 600;      // ������ֱ�ָ���λ��
int g_nVSplitBottomPos = 600;   // �ײ���ֱ�ָ���λ��

// �ָ���λ�ñ���
double g_dHSplitRatio = 0.5;     // ˮƽ�ָ���λ�ñ���
double g_dVSplitTopRatio = 0.5;  // ������ֱ�ָ���λ�ñ���
double g_dVSplitBottomRatio = 0.5; // �ײ���ֱ�ָ���λ�ñ���

const int SPLITTER_SIZE = 10;    // �ָ�����ϸ
const int SCROLLBAR_WIDTH = 20;  // �ӿ���������ڿɼ�
const int THUMB_MIN_HEIGHT = 10; // �Ӵ󻬿���ڵ��
const int SCROLL_MARGIN = 2;     // �������ڱ߾�

// ȷ��������ֻע��һ�Σ��״ε���ʱע�ᣩ
static bool g_splitMainClassRegistered = false;
static bool g_splitLineClassRegistered = false;
static bool g_scrollbarClassRegistered = false;

int g_nDragging = 0;            // 0=δ�϶�, 1=ˮƽ, 2=������ֱ, 3=�ײ���ֱ, 4=����������, 5=�ײ�������
#define TIMER_ID_RESIZE 11      //��ʱ��

// ȫ�ֱ���
HWINEVENTHOOK g_hMoveSizeHook = NULL;  // �����ƶ��Ĺ���
HWINEVENTHOOK g_hMoveEndHook = NULL;   // �ƶ������Ĺ���
HWND g_hDraggingPuTTY = NULL;          // putty�����϶�״̬
int g_nStartRegion = 0;                // �����ƶ���ʼλ��

// ������ر�������������״̬���϶���Ϣ��
int g_nScrollPosTop = 0;        // ����������λ��
int g_nScrollPosBottom = 0;     // �ײ�������λ��
int g_nContentHeightTop = 10000; // �������ݸ߶�
int g_nContentHeightBottom = 10000; // �ײ����ݸ߶�
int g_nVisibleHeightTop = 0;    // �����ɼ��߶�
int g_nVisibleHeightBottom = 0; // �ײ��ɼ��߶�
int g_nScrollRangeTop = 0;      // �����ɹ�����Χ
int g_nScrollRangeBottom = 0;   // �ײ��ɹ�����Χ

// ������״̬ö�٣�������
typedef enum {
	SB_STATE_NORMAL,   // ����
	SB_STATE_HOVER,    // ��ͣ
	SB_STATE_DRAGGING  // �϶���
} ScrollBarState;

// ������˽�����ݣ��洢ÿ����������״̬����Դ��֧�ֶ����������
typedef struct {
	ScrollBarState thumbState;  // ����״̬
	BOOL isDragging;            // �Ƿ������϶�
	int dragYOffset;            // �϶�ʱ����ڻ����ڵ�Yƫ��
	// ��ͼ��Դ�������ظ��������٣�
	HBRUSH hTrackBrush;         // �����ˢ��ǳ�ң�
	HBRUSH hThumbNormalBrush;   // ����������ˢ��������
	HBRUSH hThumbHoverBrush;    // ������ͣ��ˢ��������
	HBRUSH hBorderBrush;        // �߿�ˢ����ɫ��
} ScrollBarData;
static ScrollBarData g_scrollTopData; // ��������������
static ScrollBarData g_scrollBottomData; // �ײ�����������

// ע�ᴰ����
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

		// ����ע�ᴰ����
		ATOM atom = RegisterClassEx(&wc);
		DWORD error = GetLastError();

		// ����Ƿ�ע��ɹ������Ѵ���
		if (!atom && error != ERROR_CLASS_ALREADY_EXISTS) {
			MessageBoxW(NULL, GetString(IDS_REGISTER_WNDCLASS_FAIL), GetString(IDS_MESSAGE_CAPTION), MB_OK | MB_ICONERROR);
			return;
		}
		g_splitMainClassRegistered = true; // ���Ϊ��ע�ᣨ���۱����Ƿ�ʵ��ע�ᣩ
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


		// ����ע�ᴰ����
		ATOM atom = RegisterClassEx(&splitterWC);
		DWORD error = GetLastError();

		// ����Ƿ�ע��ɹ������Ѵ���
		if (!atom && error != ERROR_CLASS_ALREADY_EXISTS) {
			MessageBoxW(NULL, GetString(IDS_REGISTER_WNDCLASS_FAIL), GetString(IDS_MESSAGE_CAPTION), MB_OK | MB_ICONERROR);
			return;
		}
		g_splitLineClassRegistered = true; // ���Ϊ��ע�ᣨ���۱����Ƿ�ʵ��ע�ᣩ
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

		// ����ע�ᴰ����
		ATOM atom = RegisterClassEx(&scrollWC);
		DWORD error = GetLastError();

		// ����Ƿ�ע��ɹ������Ѵ���
		if (!atom && error != ERROR_CLASS_ALREADY_EXISTS) {
			MessageBoxW(NULL, GetString(IDS_REGISTER_WNDCLASS_FAIL), GetString(IDS_MESSAGE_CAPTION), MB_OK | MB_ICONERROR);
			return;
		}
		g_scrollbarClassRegistered = true; // ���Ϊ��ע�ᣨ���۱����Ƿ�ʵ��ע�ᣩ
	}
}

// ����������
HWND createSplitWindow(HINSTANCE hInstance, HWND appWindow) {
	g_appInstance = hInstance;
	if (g_hWndMain) {
		// ��鴰���Ƿ�����С��״̬
		if (IsIconic(g_hWndMain)) {
			// ��ԭ���ڣ�����С��״̬�ָ���
			ShowWindow(g_hWndMain, SW_RESTORE);
		}
		SetForegroundWindow(g_hWndMain);
		return g_hWndMain;
	}
	// ע�ᴰ���࣬�ڲ��ж�ֻ��ע��һ��
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

	// ��ʼ��PuTTY�϶���������
	StartPuTTYHooks();
	return g_hWndHost;
}

// ���㻬��߶ȣ����ݹ�����Χ�Ϳɼ��߶ȱ�����
static int CalcThumbHeight(int scrollRange, int visibleHeight, int clientHeight) {
	if (scrollRange <= 0) return clientHeight; // �޹���ʱ����ռ�����
	int thumbH = (int)((double)visibleHeight / (visibleHeight + scrollRange) * clientHeight);
	return max(thumbH, THUMB_MIN_HEIGHT);
}

// ���㻬��Y���꣨���ݵ�ǰ����λ�ã�
static int CalcThumbY(int scrollPos, int scrollRange, int thumbHeight, int clientHeight) {
	if (scrollRange <= 0) return 0; // �޹���ʱ�����ö�
	int trackAvailH = clientHeight - thumbHeight; // ������ø߶�
	return (int)((double)scrollPos / scrollRange * trackAvailH);
}

// �����λ�ü������λ�ã��϶�ʱ�ã�
static int CalcPosFromMouse(int mouseY, int dragOffset, int scrollRange, int thumbHeight, int clientHeight) {
	if (scrollRange <= 0) return 0;
	int trackAvailH = clientHeight - thumbHeight;
	if (trackAvailH <= 0) return 0;
	// ����ڹ���е����Y���� = ���Y - �϶�ƫ�� - �ͻ�������
	int relY = mouseY - dragOffset;
	// �����ڹ����Χ��
	relY = max(0, min(relY, trackAvailH));
	return (int)((double)relY / trackAvailH * scrollRange);
}

// ��ʼ����������Դ�������ظ�������
static void InitScrollBarData(ScrollBarData* sbData) {
	if (sbData->hTrackBrush) return; // �ѳ�ʼ����

	// ��ʼ����ͼ��Դ����������/��ˢ��
	sbData->hTrackBrush = CreateSolidBrush(RGB(230, 230, 230));       // �����ǳ��
	sbData->hThumbNormalBrush = CreateSolidBrush(RGB(100, 180, 255)); // ��������������
	sbData->hThumbHoverBrush = CreateSolidBrush(RGB(50, 150, 255));   // ������ͣ������
	sbData->hBorderBrush = CreateSolidBrush(RGB(0, 0, 0));            // �߿򣺺�ɫ
	// ��ʼ��״̬����
	sbData->thumbState = SB_STATE_NORMAL;
	sbData->isDragging = FALSE;
	sbData->dragYOffset = 0;
}

// �ͷŹ�������Դ����ֹ�ڴ�й©��
static void FreeScrollBarData(ScrollBarData* sbData) {
	if (sbData->hTrackBrush) DeleteObject(sbData->hTrackBrush);
	if (sbData->hThumbNormalBrush) DeleteObject(sbData->hThumbNormalBrush);
	if (sbData->hThumbHoverBrush) DeleteObject(sbData->hThumbHoverBrush);
	if (sbData->hBorderBrush) DeleteObject(sbData->hBorderBrush);
	ZeroMemory(sbData, sizeof(ScrollBarData));
}

// ���¹�����Χ
void UpdateScrollRanges() {
	// ���㶥�����������Χ
	g_nVisibleHeightTop = g_nHSplitPos;
	g_nScrollRangeTop = max(0, g_nContentHeightTop - g_nVisibleHeightTop);

	// ����ײ����������Χ
	g_nVisibleHeightBottom = g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE);
	g_nScrollRangeBottom = max(0, g_nContentHeightBottom - g_nVisibleHeightBottom);

	// ȷ������λ������Ч��Χ��
	g_nScrollPosTop = min(g_nScrollPosTop, g_nScrollRangeTop);
	g_nScrollPosBottom = min(g_nScrollPosBottom, g_nScrollRangeBottom);

	// ������λ�ñ仯��ǿ���ػ�
	if (g_hScrollTop) InvalidateRect(g_hScrollTop, NULL, FALSE);
	if (g_hScrollBottom) InvalidateRect(g_hScrollBottom, NULL, FALSE);

	LOG_DEBUG(L"split scroll pos g_nScrollPosTop", g_nScrollPosTop);
}

// ��PuTTY���ڷ��͹�����Ϣ����deltaY���������������ٶȣ�
void SendScrollMessageToPuTTY(HWND hWnd, int deltaY) {

	if (!hWnd || !IsWindow(hWnd)) return;

	// �������ǿ�ȣ�����deltaY����ֵȷ������������
	// deltaY����ֵԽ�󣬹�������Խ�ࣨ�ٶ�Խ�죩
	int scrollAmount = abs(deltaY);
	// ÿ�λ���������Ӧ��deltaֵ���ɸ�����Ҫ���������ȣ�
	const int BASE_DELTA = 8;
	// �����������������1�Σ����10�α���������죩
	int scrollCount = max(1, min(scrollAmount / BASE_DELTA, 10));

	// ȷ�����������Ϲ�/�¹���
	BOOL isUp = deltaY > 0;
	WPARAM wheelParam = isUp ? (-WHEEL_DELTA << 16) : (WHEEL_DELTA << 16);
	WORD vkCode = isUp ? VK_UP : VK_DOWN;

	LOG_DEBUG(L"split send to hwnd %p %d %d", hWnd, deltaY, scrollCount);
	// ����deltaY��С���Ͷ�ι�����Ϣ
	for (int i = 0; i < scrollCount; i++) {
		// ������������Ϣ
		SendMessage(hWnd, WM_MOUSEWHEEL, wheelParam, MAKELPARAM(10, 10));

		// ���ͼ��̹�����Ϣ��Ϊ����
		//SendMessage(hWnd, WM_KEYDOWN, vkCode, 0);
		// ��΢�ӳ�ȷ����������ȷʶ�𣨿ɸ�����Ҫ������
		//Sleep(5);
		//SendMessage(hWnd, WM_KEYUP, vkCode, 0);
	}
}


// ͬ����������
void SyncScroll(int pos, bool isTop) {
	int oldPos = isTop ? g_nScrollPosTop : g_nScrollPosBottom;
	int scrollRange = isTop ? g_nScrollRangeTop : g_nScrollRangeBottom;

	// ���ƹ���λ������Ч��Χ��
	pos = max(0, min(pos, scrollRange));
	if (pos == oldPos) return; // û�б仯�򷵻�
	int deltaY = pos - oldPos; // ��������仯��

	// �ƶ���Ӧ�����PuTTY���ڲ����͹�����Ϣ
	if (isTop) {
		g_nScrollPosTop = pos;
		if (puttyHandle1 && IsWindow(puttyHandle1)) {
			SendScrollMessageToPuTTY(puttyHandle1, deltaY);
		}
		if (puttyHandle2 && IsWindow(puttyHandle2)) {
			SendScrollMessageToPuTTY(puttyHandle2, deltaY);
		}
		// ͬ�������������ػ�
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
		// ͬ���ײ��������ػ�
		if (g_hScrollBottom) InvalidateRect(g_hScrollBottom, NULL, FALSE);
	}
}

// �������ڹ���
LRESULT CALLBACK SplitWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	if (msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_MOUSEWHEEL)
	{
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		if (msg == WM_MOUSEWHEEL) {
			// ת��������Ϣ������
			ScreenToClient(hWnd, &pt);
		}
	}
	switch (msg)
	{
	case WM_ERASEBKGND: {
		// ֱ�ӷ���TRUE��ʾ�����Ѿ������˱�������
		// �������Ա���ϵͳĬ�ϵı�����������������putty�رպͱ�ǩ�л�ʱ����˸
		return TRUE;
	}
	case WM_CREATE:
	{
		CreateChildWindows(hWnd);
		// ��ʼ�����������ݣ�������ͼ��Դ��
		InitScrollBarData(&g_scrollTopData);
		InitScrollBarData(&g_scrollBottomData);
		break;
	}
	case WM_SIZE: {
		GetClientRect(g_hWndMain, &g_rcMain);
		// �����ָ���λ��
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
		// �����϶�
		if (g_nDragging > 0 && g_nDragging <= 3)
		{
			// ���·ָ���λ��
			switch (g_nDragging)
			{
			case 1: // ˮƽ�ָ���
				g_nHSplitPos = pt.y;
				g_nHSplitPos = min(max(g_nHSplitPos, 10), g_rcMain.bottom - 10);
				g_dHSplitRatio = (double)g_nHSplitPos / g_rcMain.bottom;
				break;
			case 2: // ������ֱ�ָ���
				g_nVSplitTopPos = pt.x;
				g_nVSplitTopPos = min(max(g_nVSplitTopPos, 10), g_rcMain.right - 10 - SCROLLBAR_WIDTH);
				g_dVSplitTopRatio = (double)g_nVSplitTopPos / g_rcMain.right;
				break;
			case 3: // �ײ���ֱ�ָ���
				g_nVSplitBottomPos = pt.x;
				g_nVSplitBottomPos = min(max(g_nVSplitBottomPos, 10), g_rcMain.right - 10 - SCROLLBAR_WIDTH);
				g_dVSplitBottomRatio = (double)g_nVSplitBottomPos / g_rcMain.right;
				break;
			}
			// ͨ����ʱ�����ٴ����ػ�������û���仯��ƽ��
			SetTimer(hWnd, TIMER_ID_RESIZE, 10, NULL);
			return 0;
		}
		// �����ͣʱ�ı��� - ��ǿ�ָ����ɽ�������ʾ
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
			KillTimer(hWnd, TIMER_ID_RESIZE); // �رռ�ʱ��
			ArrangeWindows();
			UpdateScrollRanges();
		}
		return 0;
	}
	case WM_LBUTTONDOWN:
		// ��ʼ�϶�
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
		// �����϶�
		if (g_nDragging > 0 && g_nDragging <= 3)
		{
			g_nDragging = 0;
			ReleaseCapture();
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			return 0;
		}
		break;
	case WM_MOUSEWHEEL: {
		// �¹�Ϊ�����Ϲ�Ϊ��
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

		// �ж�������ĸ�����
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
		// ���ñ���
		g_dHSplitRatio = 0.5;
		g_dVSplitTopRatio = 0.5;
		g_dVSplitBottomRatio = 0.5;
		// ����putty���
		puttyHandle1 = NULL;
		puttyHandle2 = NULL;
		puttyHandle3 = NULL;
		puttyHandle4 = NULL;
		// ж���϶���������
		StopPuTTYHooks();
		// �ͷŹ�������Դ
		FreeScrollBarData(&g_scrollTopData);
		FreeScrollBarData(&g_scrollBottomData);
		return 0;
	}
	}

	return DefWindowProcW(hWnd, msg, wParam, lParam);
}


// Ƕ��PuTTY���ڵ�������
void insertSplitWindow(HWND puttyHwnd, int pos) {
	if (!puttyHwnd || !IsWindow(puttyHwnd)) return;

	// ����ɾ����������
	HWND oldHandle = NULL;
	switch (pos) {
	case 1: oldHandle = puttyHandle1; puttyHandle1 = puttyHwnd; break;
	case 2: oldHandle = puttyHandle2; puttyHandle2 = puttyHwnd; break;
	case 3: oldHandle = puttyHandle3; puttyHandle3 = puttyHwnd; break;
	case 4: oldHandle = puttyHandle4; puttyHandle4 = puttyHwnd; break;
	default: oldHandle = puttyHandle1; puttyHandle1 = puttyHwnd; break;
	}

	// ����ɴ��ڵĸ����ڹ���
	if (oldHandle && IsWindow(oldHandle)) {
		SetParent(oldHandle, NULL);
	}

	ArrangeWindows();
}

// �����Ӵ���
void CreateChildWindows(HWND hWnd)
{
	g_hWndMain = hWnd;

	// ������������
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

	// �����ָ��������ڶ�����ȷ���ɵ����
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

	// �����Զ���ͬ�������������WS_BORDERȷ���ɼ���
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

	// ���������Ƿ񴴽��ɹ�
	if (!g_hScrollTop || !g_hScrollBottom) {
		MessageBoxW(NULL, L"����������ʧ��", GetString(IDS_MESSAGE_CAPTION), MB_OK | MB_ICONERROR);
	}
}

// ���������������������ε��ص����
int CalculateOverlapArea(const RECT* rc1, const RECT* rc2) {
	int left = max(rc1->left, rc2->left);
	int right = min(rc1->right, rc2->right);
	int top = max(rc1->top, rc2->top);
	int bottom = min(rc1->bottom, rc2->bottom);

	// ���ص�
	if (left >= right || top >= bottom) {
		return 0;
	}

	// �ص���� = �� * ��
	return (right - left) * (bottom - top);
}

// �Ż���������жϣ�����50%��������ڸ��������ж�����
int GetWindowRegion(HWND hWnd) {
	if (!hWnd || !IsWindow(hWnd) || !g_hWndMain) return 0;

	RECT rcWnd, rcMain;
	GetWindowRect(hWnd, &rcWnd);
	ScreenToClient(g_hWndMain, (LPPOINT)&rcWnd);  // ת�����ڿͻ�������
	ScreenToClient(g_hWndMain, ((LPPOINT)&rcWnd) + 1);
	GetClientRect(g_hWndMain, &rcMain);
	int scrollPos = rcMain.right - SCROLLBAR_WIDTH;

	// ���㴰�������
	int wndWidth = rcWnd.right - rcWnd.left;
	int wndHeight = rcWnd.bottom - rcWnd.top;
	int totalArea = wndWidth * wndHeight;
	if (totalArea == 0) return 0;  // ��Ч����

	// ����4��Ŀ������
	RECT rcRegion1 = { 0, 0, g_nVSplitTopPos, g_nHSplitPos };
	RECT rcRegion2 = { g_nVSplitTopPos + SPLITTER_SIZE, 0, scrollPos, g_nHSplitPos };
	RECT rcRegion3 = { 0, g_nHSplitPos + SPLITTER_SIZE, g_nVSplitBottomPos, rcMain.bottom };
	RECT rcRegion4 = { g_nVSplitBottomPos + SPLITTER_SIZE, g_nHSplitPos + SPLITTER_SIZE, scrollPos, rcMain.bottom };

	// ������ÿ��������ص����
	int area1 = CalculateOverlapArea(&rcWnd, &rcRegion1);
	int area2 = CalculateOverlapArea(&rcWnd, &rcRegion2);
	int area3 = CalculateOverlapArea(&rcWnd, &rcRegion3);
	int area4 = CalculateOverlapArea(&rcWnd, &rcRegion4);

	// �����ȫ����ĳһ�����90%
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

	// �ҵ�����ص�����
	int maxArea = max(max(area1, area2), max(area3, area4));

	// ֻ�е�����ص�������������������50%ʱ���ж�����
	if (maxArea > totalArea / 2) {
		if (maxArea == area1) return 1;
		if (maxArea == area2) return 2;
		if (maxArea == area3) return 3;
		if (maxArea == area4) return 4;
	}

	return 0;  // δ����50%�ص�����
}

// �������������������ȡ��ӦPuTTY���
HWND GetHandleByRegion(int region) {
	switch (region) {
	case 1: return puttyHandle1;
	case 2: return puttyHandle2;
	case 3: return puttyHandle3;
	case 4: return puttyHandle4;
	default: return NULL;
	}
}

// �������������������Ӧ��PuTTY���
void SetHandleByRegion(int region, HWND hWnd) {
	// �������Ч�ľ�������
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

// �����������������������PuTTY���
void SwapRegionHandles(int regionA, int regionB) {
	HWND hA = GetHandleByRegion(regionA);
	HWND hB = GetHandleByRegion(regionB);
	SetHandleByRegion(regionA, hB);
	SetHandleByRegion(regionB, hA);
	LOG_DEBUG(L"Swap PuTTY: Region%d <-> Region%d", regionA, regionB);
}


// �жϴ����Ƿ�Ϊ���ǹ����PuTTY����
BOOL IsManagedPuTTY(HWND hWnd) {
	return hWnd == puttyHandle1 || hWnd == puttyHandle2 ||
		hWnd == puttyHandle3 || hWnd == puttyHandle4;
}

// ���ӻص�������ֻ��ע�ƶ�/������С�����¼���
void CALLBACK MoveSizeChangeHookProc(
	HWINEVENTHOOK hHook, DWORD event,
	HWND hWnd, LONG idObject, LONG idChild,
	DWORD idEventThread, DWORD dwmsEventTime
) {
	// ֻ�����ڱ����������ǵ�PuTTY����
	if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF)
		return;
	if (!IsManagedPuTTY(hWnd))
		return;

	if (event == EVENT_SYSTEM_MOVESIZESTART) {
		if (!g_hDraggingPuTTY) { // �״δ�����Ϊ�϶���ʼ
			g_hDraggingPuTTY = hWnd;
			g_nStartRegion = GetWindowRegion(hWnd); // ��¼��ʼ����
			LOG_DEBUG(L"move start %d %p", g_nStartRegion, hWnd);
		}
		return;
	}
	if (event == EVENT_SYSTEM_MOVESIZEEND) {
		if (g_hDraggingPuTTY == hWnd) {
			int endRegion = GetWindowRegion(hWnd);
			if (endRegion != 0 && endRegion != g_nStartRegion) {
				// ִ�о�������߼�
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
			g_hDraggingPuTTY = NULL;  // �����϶�״̬
			g_nStartRegion = 0;
		}
	}
}

// �������ӣ�ͬʱ����λ�ñ仯���ƶ�������
void StartPuTTYHooks() {
	StopPuTTYHooks();

	// 2�����Ӳ��ܺϳ�һ�������ղ�����Ϣ
	// ע��λ�ñ仯����
	g_hMoveSizeHook = SetWinEventHook(
		EVENT_SYSTEM_MOVESIZESTART, EVENT_SYSTEM_MOVESIZESTART,  // ����λ�ñ仯���϶���ʼ��
		NULL, MoveSizeChangeHookProc,                            // �ص�����
		0, 0,                                                    // ���н��̡������߳�
		WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS          // ����̼������������Ĺ���
	);

	// ע���ƶ���������
	g_hMoveEndHook = SetWinEventHook(
		EVENT_SYSTEM_MOVESIZEEND, EVENT_SYSTEM_MOVESIZEEND, // �����ƶ��������϶�������
		NULL, MoveSizeChangeHookProc,
		0, 0,
		WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
	);
}

// ֹͣ����
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

// ���д���
void ArrangeWindows() {
	GetClientRect(g_hWndMain, &g_rcMain);
	if (g_rcMain.right == 0 || g_rcMain.bottom == 0)
		return;
	LOG_DEBUG(L"Arrange %p %p %p %p", puttyHandle1, puttyHandle2, puttyHandle3, puttyHandle4);
	int scrollPos = g_rcMain.right - SCROLLBAR_WIDTH;

	// �������д���λ�ã�����ԭ���߼���
	// ����1 - ����
	if (puttyHandle1 && IsWindow(puttyHandle1)) {
		// ����һ����϶�����㶨λ�϶���
		MoveWindow(puttyHandle1, 0, 0, g_nVSplitTopPos-1, g_nHSplitPos, TRUE);
	}

	// ������ֱ�ָ���
	MoveWindow(g_hWndVSplitTop, g_nVSplitTopPos, 0, SPLITTER_SIZE, g_nHSplitPos, TRUE);

	// ����2 - ����
	if (puttyHandle2 && IsWindow(puttyHandle2)) {
		MoveWindow(puttyHandle2, g_nVSplitTopPos + SPLITTER_SIZE, 0,
			scrollPos - (g_nVSplitTopPos + SPLITTER_SIZE), g_nHSplitPos, TRUE);
	}

	// ˮƽ�ָ���
	MoveWindow(g_hWndHSplit, 0, g_nHSplitPos, g_rcMain.right, SPLITTER_SIZE, TRUE);

	// ����3 - ����
	if (puttyHandle3 && IsWindow(puttyHandle3)) {
		// ����һ����϶�����㶨λ�϶���
		MoveWindow(puttyHandle3, 0, g_nHSplitPos + SPLITTER_SIZE,
			g_nVSplitBottomPos - 1, g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE), TRUE);
	}

	// �ײ���ֱ�ָ���
	MoveWindow(g_hWndVSplitBottom,
		g_nVSplitBottomPos, g_nHSplitPos + SPLITTER_SIZE,
		SPLITTER_SIZE, g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE),
		TRUE);

	// ����4 - ����
	if (puttyHandle4 && IsWindow(puttyHandle4)) {
		MoveWindow(puttyHandle4,
			g_nVSplitBottomPos + SPLITTER_SIZE, g_nHSplitPos + SPLITTER_SIZE,
			scrollPos - (g_nVSplitBottomPos + SPLITTER_SIZE),
			g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE),
			TRUE);
	}

	// ����������λ�ã�ȷ���ɼ���
	if (g_hScrollTop) {
		MoveWindow(g_hScrollTop, scrollPos, 0, SCROLLBAR_WIDTH, g_nHSplitPos, TRUE);
		ShowWindow(g_hScrollTop, SW_SHOW); // ǿ����ʾ
	}
	if (g_hScrollBottom) {
		MoveWindow(g_hScrollBottom, scrollPos, g_nHSplitPos + SPLITTER_SIZE,
			SCROLLBAR_WIDTH, g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE), TRUE);
		ShowWindow(g_hScrollBottom, SW_SHOW); // ǿ����ʾ
	}
	LOG_DEBUG(L"ArrangeWindows: rc=%d, %d %d %d", g_rcMain.left, g_rcMain.top, g_rcMain.right, g_rcMain.bottom);
}

// �ָ������ڹ���
LRESULT CALLBACK SplitterProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// ת�����겢ת�������Ϣ��������
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

// �Զ�����������ڹ��̣��޸��棩
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

	// �������λ��
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

		// �����ڴ�DC����˫����
		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBitmap = CreateCompatibleBitmap(hdc,
			clientRect.right - clientRect.left,
			clientRect.bottom - clientRect.top);
		HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap);

		HGDIOBJ oldBrush;

		// 1. ���ƹ���������
		HBRUSH bgBrush = CreateSolidBrush(RGB(255, 240, 240));
		oldBrush = SelectObject(memDC, bgBrush);
		Rectangle(memDC, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
		SelectObject(memDC, oldBrush);
		DeleteObject(bgBrush);

		// 2. ���ƹ��
		RECT trackRect = {
			clientRect.left + SCROLL_MARGIN,
			clientRect.top + SCROLL_MARGIN,
			clientRect.right - SCROLL_MARGIN,
			clientRect.bottom - SCROLL_MARGIN
		};
		oldBrush = SelectObject(memDC, sbData->hTrackBrush);
		Rectangle(memDC, trackRect.left, trackRect.top, trackRect.right, trackRect.bottom);
		SelectObject(memDC, oldBrush);

		// 3. ���ƻ���
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

		// 4. ���ƻ���߿�
		HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
		HPEN oldPen = (HPEN)SelectObject(memDC, borderPen);
		MoveToEx(memDC, thumbRect.left, thumbRect.top, NULL);
		LineTo(memDC, thumbRect.right, thumbRect.top);
		LineTo(memDC, thumbRect.right, thumbRect.bottom);
		LineTo(memDC, thumbRect.left, thumbRect.bottom);
		LineTo(memDC, thumbRect.left, thumbRect.top);
		SelectObject(memDC, oldPen);
		DeleteObject(borderPen);

		// ���ڴ�DC����һ���Ը��Ƶ���ĻDC
		BitBlt(hdc, clientRect.left, clientRect.top,
			clientRect.right - clientRect.left,
			clientRect.bottom - clientRect.top,
			memDC, 0, 0, SRCCOPY);

		// ������Դ
		SelectObject(memDC, oldBitmap);
		DeleteObject(memBitmap);
		DeleteDC(memDC);
		EndPaint(hWnd, &ps);
		return 0;
	}

	case WM_ERASEBKGND:
		// ��ֹϵͳ����������������˸
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
			// ֻ��λ�ñ仯ʱ���ػ�
			if (newPos != *pPos) {
				SyncScroll(newPos, isTop);
			}
			return 0;
		}

		ScrollBarState newState = PtInRect(&thumbRect, pt) ? SB_STATE_HOVER : SB_STATE_NORMAL;
		if (newState != sbData->thumbState) {
			sbData->thumbState = newState;
			// ֻ�ػ滬��������������ؼ�
			InvalidateRect(hWnd, &thumbRect, FALSE);
			SetCursor(LoadCursor(NULL, IDC_HAND));
		}
		return 0;
	}

	case WM_LBUTTONDOWN: {
		LOG_DEBUG(L"splitter ScrollbarProc WM_LBUTTONDOWN");
		// ��������
		RECT thumbRect = {
			clientRect.left + SCROLL_MARGIN,
			clientRect.top + SCROLL_MARGIN + thumbY,
			clientRect.right - SCROLL_MARGIN,
			clientRect.top + SCROLL_MARGIN + thumbY + thumbHeight
		};

		if (PtInRect(&thumbRect, pt)) {
			// ��ʼ�϶�����
			sbData->isDragging = TRUE;
			sbData->thumbState = SB_STATE_DRAGGING;
			sbData->dragYOffset = pt.y - thumbRect.top; // ��¼����ڻ����ڵ�ƫ��
			SetCapture(hWnd);
			InvalidateRect(hWnd, NULL, FALSE);
			return 0;
		}

		// �������հ״�
		int lineStep = visibleHeight / 2; // ��ҳ����
		int newPos = *pPos;

		if (pt.y < thumbY + SCROLL_MARGIN) {
			newPos -= lineStep; // �Ϲ�
		}
		else {
			newPos += lineStep; // �¹�
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
		int lineStep = visibleHeight / 20;  // �й���
		int pageStep = visibleHeight;       // ҳ����

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
		// ʼ����ʾ���͹����ʾ�ɽ���
		SetCursor(LoadCursor(NULL, IDC_HAND));
		return 1;

	case WM_DESTROY:
		// �ͷ���Դ
		FreeScrollBarData(sbData);
		break;
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

// �������λ���ĸ��ָ����� - ���������򣬸�����ѡ��
int GetSplitterAtPoint(POINT pt)
{
	// ���ˮƽ�ָ��� - ����������10����
	RECT rcHSplit;
	rcHSplit.top = g_nHSplitPos - 5;
	rcHSplit.bottom = g_nHSplitPos + 5;
	rcHSplit.left = 0;
	rcHSplit.right = g_rcMain.right;
	if (PtInRect(&rcHSplit, pt))
		return 1;

	// ��鶥����ֱ�ָ���
	RECT rcVSplitTop;
	rcVSplitTop.left = g_nVSplitTopPos - 5;
	rcVSplitTop.right = g_nVSplitTopPos + 5;
	rcVSplitTop.top = 0;
	rcVSplitTop.bottom = g_nHSplitPos;
	if (PtInRect(&rcVSplitTop, pt))
		return 2;

	// ���ײ���ֱ�ָ���
	RECT rcVSplitBottom;
	rcVSplitBottom.left = g_nVSplitBottomPos - 5;
	rcVSplitBottom.right = g_nVSplitBottomPos + 5;
	rcVSplitBottom.top = g_nHSplitPos;
	rcVSplitBottom.bottom = g_rcMain.bottom;
	if (PtInRect(&rcVSplitBottom, pt))
		return 3;

	return 0;
}
