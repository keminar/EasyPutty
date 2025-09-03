#include "splitter.h"
#include "attach.h"
#include "logs.h"

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

// 创建子窗口
void CreateChildWindows(HINSTANCE hInstance, HWND parentWindow)
{
	GetClientRect(parentWindow, &g_rcMain);

	puttyHandle1 = createPuttyWindow(hInstance, parentWindow, L"cmd");
	MoveWindow(puttyHandle1, 0, 0, 600, 450, TRUE);
	
	puttyHandle2 = createPuttyWindow(hInstance, parentWindow, L"notepad");
	MoveWindow(puttyHandle2, 600, 0, 600, 450, TRUE);
	
	puttyHandle3 = createPuttyWindow(hInstance, parentWindow, L"cmd");
	MoveWindow(puttyHandle3, 0, 450, 600, 450, TRUE);
	
	puttyHandle4 = createPuttyWindow(hInstance, parentWindow, L"notepad");
	MoveWindow(puttyHandle4, 600, 450, 600, 450, TRUE);
	
}

void ArrangeWindows(HINSTANCE hInstance, HWND parentWindow) {
	// 保存旧的客户区大小用于计算比例
	RECT rcOld = g_rcMain;
	GetClientRect(parentWindow, &g_rcMain);

	if (g_rcMain.right == 0 || g_rcMain.bottom == 0)
		return;

	int scrollPos = g_rcMain.right - SCROLLBAR_WIDTH;

	// 调整分隔条位置
	if (rcOld.right > 0 && rcOld.bottom > 0 && g_rcMain.right > 0 && g_rcMain.bottom > 0)
	{
		g_nHSplitPos = (int)(g_dHSplitRatio * g_rcMain.bottom);
		g_nVSplitTopPos = (int)(g_dVSplitTopRatio * g_rcMain.right);
		g_nVSplitBottomPos = (int)(g_dVSplitBottomRatio * g_rcMain.right);
	}

	// 更新比例
	if (g_rcMain.bottom > 0)
		g_dHSplitRatio = (double)g_nHSplitPos / g_rcMain.bottom;
	if (g_rcMain.right > 0)
	{
		g_dVSplitTopRatio = (double)g_nVSplitTopPos / g_rcMain.right;
		g_dVSplitBottomRatio = (double)g_nVSplitBottomPos / g_rcMain.right;
	}
	
	// 窗口1 - 上左
	MoveWindow(puttyHandle1,
		0, 0,
		g_nVSplitTopPos, g_nHSplitPos,
		TRUE);
	
	// 窗口2 - 上右
	MoveWindow(puttyHandle2,
		g_nVSplitTopPos + SPLITTER_SIZE, 0,
		scrollPos - (g_nVSplitTopPos + SPLITTER_SIZE), g_nHSplitPos,
		TRUE);

	// 水平分隔条
	/*MoveWindow(g_hWndHSplit,
		0, g_nHSplitPos,
		g_rcMain.right, SPLITTER_SIZE,
		TRUE);
		*/
	// 窗口3 - 下左
	MoveWindow(puttyHandle3,
		0, g_nHSplitPos + SPLITTER_SIZE,
		g_nVSplitBottomPos, g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE),
		TRUE);

	// 窗口4 - 下右
	MoveWindow(puttyHandle4,
		g_nVSplitBottomPos + SPLITTER_SIZE, g_nHSplitPos + SPLITTER_SIZE,
		scrollPos - (g_nVSplitBottomPos + SPLITTER_SIZE), g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE),
		TRUE);


	// 顶部垂直分隔条
	/*MoveWindow(g_hWndVSplitTop,
		g_nVSplitTopPos, 0,
		SPLITTER_SIZE, g_nHSplitPos,
		TRUE);

	// 底部垂直分隔条
	MoveWindow(g_hWndVSplitBottom,
		g_nVSplitBottomPos, g_nHSplitPos + SPLITTER_SIZE,
		SPLITTER_SIZE, g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE),
		TRUE);
	*/
	//LOG_DEBUG(L"ArrangeWindows: rc=%d, %d %d %d", rc.left, rc.top, rc.right, rc.bottom);
	//MoveWindow(puttyHandle4, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
}