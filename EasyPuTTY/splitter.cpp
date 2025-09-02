#include "splitter.h"

HWND g_hWnd1, g_hWnd2, g_hWnd3, g_hWnd4; // 四个独立子窗口

// 创建子窗口
void CreateChildWindows(HINSTANCE hInstance, HWND parentWindow)
{
	g_hWnd1 = CreateWindowExW(
		0,
		L"Static",
		L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0,
		400, 300,
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
		400, 0,
		400, 300,
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
		0, 300,
		400, 300,
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
		400, 300,
		400, 300,
		parentWindow,
		NULL,
		hInstance,
		NULL
	);
	 
}