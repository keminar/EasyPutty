#include "splitter.h"
#include "attach.h"
#include "logs.h"

static HINSTANCE g_appInstance;

HWND g_hWndMain;                // �����ھ��
HWND g_hWndHSplit;              // ˮƽ�ָ���
HWND g_hWndVSplitTop;           // ������ֱ�ָ���
HWND g_hWndVSplitBottom;        // �ײ���ֱ�ָ���
HWND g_hScrollTop, g_hScrollBottom; // ͬ��������
HWND puttyHandle1, puttyHandle2, puttyHandle3, puttyHandle4;

RECT g_rcMain;                  // �����ڿͻ�������
int g_nHSplitPos = 450;         // ˮƽ�ָ���λ��
int g_nVSplitTopPos = 600;      // ������ֱ�ָ���λ��
int g_nVSplitBottomPos = 600;   // �ײ���ֱ�ָ���λ��

// �ָ���λ�ñ���
double g_dHSplitRatio = 0.5;     // ˮƽ�ָ���λ�ñ���
double g_dVSplitTopRatio = 0.5;  // ������ֱ�ָ���λ�ñ���
double g_dVSplitBottomRatio = 0.5; // �ײ���ֱ�ָ���λ�ñ���

const int SPLITTER_SIZE = 5;    // �ָ�����ϸ
const int SCROLLBAR_WIDTH = 20; // ���������

// ȷ��������ֻע��һ�Σ��״ε���ʱע�ᣩ
static bool g_splitMainClassRegistered = false;
static bool g_splitLineClassRegistered = false;
static bool g_scrollbarClassRegistered = false;

int g_nDragging = 0;            // 0=δ�϶�, 1=ˮƽ, 2=������ֱ, 3=�ײ���ֱ, 4=����������, 5=�ײ�������
#define TIMER_ID_RESIZE 11      //��ʱ��

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
	// ȷ��������ֻע��һ�Σ��״ε���ʱע�ᣩ
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

// �������ڹ���
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
		// ֱ�ӷ���TRUE��ʾ�����Ѿ������˱�������
		// �������Ա���ϵͳĬ�ϵı�����������������putty�رպͱ�ǩ�л�ʱ����˸
		return TRUE;
	}
	case WM_CREATE:
	{
		CreateChildWindows(hWnd);
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

		ArrangeWindows();
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
		}
		return 0;
	}
	case WM_LBUTTONDOWN:
		// ��ʼ�϶�
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
		// �����϶�
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
		// ���ñ���
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


// Ƕ��PuTTY���ڵ�������
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

// �����Ӵ���
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
	
	// �����Զ���ͬ��������
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

	// �����ָ��������ڶ�����ȷ���ɵ����
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

// �������д���λ��
void ArrangeWindows() {
	GetClientRect(g_hWndMain, &g_rcMain);
	if (g_rcMain.right == 0 || g_rcMain.bottom == 0)
		return;

	int scrollPos = g_rcMain.right - SCROLLBAR_WIDTH;


	// ����1 - ����
	if (puttyHandle1 && IsWindow(puttyHandle1)) {
		MoveWindow(puttyHandle1, 0, 0, g_nVSplitTopPos, g_nHSplitPos, TRUE);
	}

	// ������ֱ�ָ���
	MoveWindow(g_hWndVSplitTop, g_nVSplitTopPos, 0, SPLITTER_SIZE, g_nHSplitPos, TRUE);

	// ����2 - ����
	if (puttyHandle2 && IsWindow(puttyHandle2)) {
		MoveWindow(puttyHandle2, g_nVSplitTopPos + SPLITTER_SIZE, 0, scrollPos - (g_nVSplitTopPos + SPLITTER_SIZE), g_nHSplitPos, TRUE);
	}
	// ˮƽ�ָ���
	MoveWindow(g_hWndHSplit, 0, g_nHSplitPos, g_rcMain.right, SPLITTER_SIZE, TRUE);
		
	// ����3 - ����
	if (puttyHandle3 && IsWindow(puttyHandle3)) {
		MoveWindow(puttyHandle3, 0, g_nHSplitPos + SPLITTER_SIZE, g_nVSplitBottomPos, g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE), TRUE);
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
			scrollPos - (g_nVSplitBottomPos + SPLITTER_SIZE), g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE),
			TRUE);
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


// �Զ�����������ڹ���
LRESULT CALLBACK ScrollbarProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

// ����λ���ĸ��ָ����� - ���������򣬸�����ѡ��
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