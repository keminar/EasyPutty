#include "splitter.h"
#include "attach.h"
#include "logs.h"

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

// �����Ӵ���
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
	// ����ɵĿͻ�����С���ڼ������
	RECT rcOld = g_rcMain;
	GetClientRect(parentWindow, &g_rcMain);

	if (g_rcMain.right == 0 || g_rcMain.bottom == 0)
		return;

	int scrollPos = g_rcMain.right - SCROLLBAR_WIDTH;

	// �����ָ���λ��
	if (rcOld.right > 0 && rcOld.bottom > 0 && g_rcMain.right > 0 && g_rcMain.bottom > 0)
	{
		g_nHSplitPos = (int)(g_dHSplitRatio * g_rcMain.bottom);
		g_nVSplitTopPos = (int)(g_dVSplitTopRatio * g_rcMain.right);
		g_nVSplitBottomPos = (int)(g_dVSplitBottomRatio * g_rcMain.right);
	}

	// ���±���
	if (g_rcMain.bottom > 0)
		g_dHSplitRatio = (double)g_nHSplitPos / g_rcMain.bottom;
	if (g_rcMain.right > 0)
	{
		g_dVSplitTopRatio = (double)g_nVSplitTopPos / g_rcMain.right;
		g_dVSplitBottomRatio = (double)g_nVSplitBottomPos / g_rcMain.right;
	}
	
	// ����1 - ����
	MoveWindow(puttyHandle1,
		0, 0,
		g_nVSplitTopPos, g_nHSplitPos,
		TRUE);
	
	// ����2 - ����
	MoveWindow(puttyHandle2,
		g_nVSplitTopPos + SPLITTER_SIZE, 0,
		scrollPos - (g_nVSplitTopPos + SPLITTER_SIZE), g_nHSplitPos,
		TRUE);

	// ˮƽ�ָ���
	/*MoveWindow(g_hWndHSplit,
		0, g_nHSplitPos,
		g_rcMain.right, SPLITTER_SIZE,
		TRUE);
		*/
	// ����3 - ����
	MoveWindow(puttyHandle3,
		0, g_nHSplitPos + SPLITTER_SIZE,
		g_nVSplitBottomPos, g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE),
		TRUE);

	// ����4 - ����
	MoveWindow(puttyHandle4,
		g_nVSplitBottomPos + SPLITTER_SIZE, g_nHSplitPos + SPLITTER_SIZE,
		scrollPos - (g_nVSplitBottomPos + SPLITTER_SIZE), g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE),
		TRUE);


	// ������ֱ�ָ���
	/*MoveWindow(g_hWndVSplitTop,
		g_nVSplitTopPos, 0,
		SPLITTER_SIZE, g_nHSplitPos,
		TRUE);

	// �ײ���ֱ�ָ���
	MoveWindow(g_hWndVSplitBottom,
		g_nVSplitBottomPos, g_nHSplitPos + SPLITTER_SIZE,
		SPLITTER_SIZE, g_rcMain.bottom - (g_nHSplitPos + SPLITTER_SIZE),
		TRUE);
	*/
	//LOG_DEBUG(L"ArrangeWindows: rc=%d, %d %d %d", rc.left, rc.top, rc.right, rc.bottom);
	//MoveWindow(puttyHandle4, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
}