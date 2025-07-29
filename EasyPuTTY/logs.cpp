#include "logs.h"

HWND hDebugEdit = NULL;

// 转换日志级别到宽字符串
static const wchar_t* LogLevelToStringW(LogLevel level) {
	switch (level) {
	case LOG_LEVEL_DEBUG: return L"DEBUG";
	case LOG_LEVEL_INFO:  return L"INFO";
	case LOG_LEVEL_WARN:  return L"WARN";
	case LOG_LEVEL_ERROR: return L"ERROR";
	default:              return L"UNKNOWN";
	}
}

// 获取当前时间的宽字符串表示
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

// 日志实现，[时间戳] [日志级别] 文件名:行号 - 日志内容
void LogW(LogLevel level, const wchar_t* file, int line, const wchar_t* format, ...) {
	wchar_t timeStr[32];
	GetCurrentTimeStringW(timeStr, sizeof(timeStr) / sizeof(wchar_t));

	// 构建日志前缀
	wchar_t logPrefix[256];
	swprintf_s(logPrefix, sizeof(logPrefix) / sizeof(wchar_t),
		L"[%s] [%s] %s(%d): ",
		timeStr, LogLevelToStringW(level), file, line);

	// 处理可变参数
	va_list args;
	va_start(args, format);

	// 计算消息缓冲区大小
	int msgLen = _vscwprintf(format, args) + 1; // +1 用于终止符
	wchar_t* msgBuffer = (wchar_t*)LocalAlloc(LPTR, msgLen * sizeof(wchar_t));
	if (msgBuffer) {
		// 格式化消息
		vswprintf_s(msgBuffer, msgLen, format, args);

		// 合并前缀和消息
		int totalLen = wcslen(logPrefix) + wcslen(msgBuffer) + 2; // +2 用于换行和终止符
		wchar_t* fullMessage = (wchar_t*)LocalAlloc(LPTR, totalLen * sizeof(wchar_t));
		if (fullMessage) {
			wcscpy_s(fullMessage, totalLen, logPrefix);
			wcscat_s(fullMessage, totalLen, msgBuffer);
			wcscat_s(fullMessage, totalLen, L"\n");

			// 输出到调试器
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

// 向调试窗口添加文本
void DebugPrint(LPCTSTR text)
{
	if (hDebugEdit == NULL) return;

	// 获取当前文本长度
	int length = GetWindowTextLength(hDebugEdit);

	// 移动到文本末尾
	SendMessage(hDebugEdit, EM_SETSEL, length, length);

	// 添加新文本
	SendMessage(hDebugEdit, EM_REPLACESEL, FALSE, (LPARAM)text);

	// 添加换行
	SendMessage(hDebugEdit, EM_REPLACESEL, FALSE, (LPARAM)_T("\r\n"));
}
