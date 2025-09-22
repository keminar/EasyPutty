#pragma once
#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include "apputils.h"

// 语言类型定义
typedef enum  {
	LANG_CN,
	LANG_EN
} LanguageType;

// 全局语言设置
extern LanguageType g_currentLang;

void SetLangInstance(HINSTANCE hInstance);

// 初始化语言（根据系统设置）
void InitLanguage();

// 获取字符串资源
const wchar_t* GetString(int stringId);
const wchar_t* GetStringBrowser(int stringId);

// 格式化字符串（支持带参数的多语言文本）
int FormatString(wchar_t* buffer, size_t bufferSize, int stringId, ...);

LPCWSTR MakeIntreSource(int cn, int en);