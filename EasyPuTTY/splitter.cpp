#include "splitter.h"
#include "attach.h"

HWND g_hWnd1, g_hWnd2, g_hWnd3, g_hWnd4; // 四个独立子窗口

// 创建子窗口
void CreateChildWindows(HINSTANCE hInstance, HWND parentWindow)
{
	RECT rc;
	g_hWnd1 = CreateWindowExW(
		0,
		L"Static",
		L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0,
		600, 450,
		parentWindow,
		NULL,
		hInstance,
		NULL
	);

	g_hWnd2 = CreateWindowExW(
		0,
		L"Static",
		L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		600, 0,
		600, 450,
		parentWindow,
		NULL,
		hInstance,
		NULL
	);

	g_hWnd3 = CreateWindowExW(
		0,
		L"Static",
		L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 450,
		600, 450,
		parentWindow,
		NULL,
		hInstance,
		NULL
	);

	g_hWnd4 = CreateWindowExW(
		0,
		L"Static",
		L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		600, 450,
		600, 450,
		parentWindow,
		NULL,
		hInstance,
		NULL
	);

	HWND puttyHandle1 = createPuttyWindow(hInstance, g_hWnd1, L"cmd");
	GetClientRect(g_hWnd1, &rc);
	MoveWindow(puttyHandle1, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
	
	HWND puttyHandle2 = createPuttyWindow(hInstance, g_hWnd2, L"notepad");
	GetClientRect(g_hWnd2, &rc);
	MoveWindow(puttyHandle2, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
	
	HWND puttyHandle3 = createPuttyWindow(hInstance, g_hWnd3, L"cmd");
	GetClientRect(g_hWnd3, &rc);
	MoveWindow(puttyHandle3, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
	
	HWND puttyHandle4 = createPuttyWindow(hInstance, g_hWnd4, L"notepad");
	GetClientRect(g_hWnd4, &rc);
	MoveWindow(puttyHandle4, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
	
}

void ArrangeWindows(HINSTANCE hInstance, HWND parentWindow) {

}