#include "logs.h"

HWND hDebugEdit = NULL;

// ת����־���𵽿��ַ���
static const wchar_t* LogLevelToStringW(LogLevel level) {
	switch (level) {
	case LOG_LEVEL_DEBUG: return L"DEBUG";
	case LOG_LEVEL_INFO:  return L"INFO";
	case LOG_LEVEL_WARN:  return L"WARN";
	case LOG_LEVEL_ERROR: return L"ERROR";
	default:              return L"UNKNOWN";
	}
}

// ��ȡ��ǰʱ��Ŀ��ַ�����ʾ
static void GetCurrentTimeStringW(wchar_t* buffer, size_t bufferSize) {
	time_t now;
	time(&now);
	struct tm localTime;
	localtime_s(&localTime, &now);

	swprintf_s(buffer, bufferSize, L"%04d-%02d-%02d %02d:%02d:%02d",
		localTime.tm_year + 1900,
		localTime.tm_mon + 1,
		localTime.tm_mday,
		localTime.tm_hour,
		localTime.tm_min,
		localTime.tm_sec);
}

// ��־ʵ�֣�[ʱ���] [��־����] �ļ���:�к� - ��־����
void LogW(LogLevel level, const wchar_t* file, int line, const wchar_t* format, ...) {
	wchar_t timeStr[32];
	GetCurrentTimeStringW(timeStr, sizeof(timeStr) / sizeof(wchar_t));

	// ������־ǰ׺
	wchar_t logPrefix[256];
	swprintf_s(logPrefix, sizeof(logPrefix) / sizeof(wchar_t),
		L"[%s] [%s] %s(%d): ",
		timeStr, LogLevelToStringW(level), file, line);

	// ����ɱ����
	va_list args;
	va_start(args, format);

	// ������Ϣ��������С
	int msgLen = _vscwprintf(format, args) + 1; // +1 ������ֹ��
	wchar_t* msgBuffer = (wchar_t*)LocalAlloc(LPTR, msgLen * sizeof(wchar_t));
	if (msgBuffer) {
		// ��ʽ����Ϣ
		vswprintf_s(msgBuffer, msgLen, format, args);

		// �ϲ�ǰ׺����Ϣ
		int totalLen = wcslen(logPrefix) + wcslen(msgBuffer) + 2; // +2 ���ڻ��к���ֹ��
		wchar_t* fullMessage = (wchar_t*)LocalAlloc(LPTR, totalLen * sizeof(wchar_t));
		if (fullMessage) {
			wcscpy_s(fullMessage, totalLen, logPrefix);
			wcscat_s(fullMessage, totalLen, msgBuffer);
			wcscat_s(fullMessage, totalLen, L"\n");

			// �����������
			OutputDebugStringW(fullMessage);
			DebugPrint(fullMessage);

			LocalFree(fullMessage);
		}
		LocalFree(msgBuffer);
	}
	va_end(args);
}

void setEditHwnd(HWND h) {
	hDebugEdit = h;
}

// ����Դ�������ı�
void DebugPrint(LPCTSTR text)
{
	if (hDebugEdit == NULL) return;

	// ��ȡ��ǰ�ı�����
	int length = GetWindowTextLength(hDebugEdit);

	// �ƶ����ı�ĩβ
	SendMessage(hDebugEdit, EM_SETSEL, length, length);

	// ������ı�
	SendMessage(hDebugEdit, EM_REPLACESEL, FALSE, (LPARAM)text);

	// ��ӻ���
	SendMessage(hDebugEdit, EM_REPLACESEL, FALSE, (LPARAM)_T("\r\n"));
}
