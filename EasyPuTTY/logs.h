#ifndef LOGS_H
#define LOGS_H

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <tchar.h>

void setEditHwnd(HWND h);
void DebugPrint(LPCTSTR text);

// 日志级别枚举
typedef enum {
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR
} LogLevel;

// 宽字节日志函数
void LogW(LogLevel level, const wchar_t* file, int line, const wchar_t* format, ...);

// 日志宏定义 - 自动传递文件名和行号
#define LOG_DEBUG(format, ...) do { \
    wchar_t wideFile[MAX_PATH]; \
    MultiByteToWideChar(CP_ACP, 0, __FILE__, -1, wideFile, MAX_PATH); \
    LogW(LOG_LEVEL_DEBUG, wideFile, __LINE__, format, __VA_ARGS__); \
} while(0)

#define LOG_INFO(format, ...) do { \
    wchar_t wideFile[MAX_PATH]; \
    MultiByteToWideChar(CP_ACP, 0, __FILE__, -1, wideFile, MAX_PATH); \
    LogW(LOG_LEVEL_INFO, wideFile, __LINE__, format, __VA_ARGS__); \
} while(0)

#define LOG_WARN(format, ...) do { \
    wchar_t wideFile[MAX_PATH]; \
    MultiByteToWideChar(CP_ACP, 0, __FILE__, -1, wideFile, MAX_PATH); \
    LogW(LOG_LEVEL_WARN, wideFile, __LINE__, format, __VA_ARGS__); \
} while(0)

#define LOG_ERROR(format, ...) do { \
    wchar_t wideFile[MAX_PATH]; \
    MultiByteToWideChar(CP_ACP, 0, __FILE__, -1, wideFile, MAX_PATH); \
    LogW(LOG_LEVEL_ERROR, wideFile, __LINE__, format, __VA_ARGS__); \
} while(0)

#endif // LOGS_H
