#pragma once
#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include "apputils.h"

// �������Ͷ���
typedef enum  {
	LANG_CN,
	LANG_EN
} LanguageType;

// ȫ����������
extern LanguageType g_currentLang;

void SetLangInstance(HINSTANCE hInstance);

// ��ʼ�����ԣ�����ϵͳ���ã�
void InitLanguage();

// ��ȡ�ַ�����Դ
const wchar_t* GetString(int stringId);
const wchar_t* GetStringBrowser(int stringId);

// ��ʽ���ַ�����֧�ִ������Ķ������ı���
int FormatString(wchar_t* buffer, size_t bufferSize, int stringId, ...);

LPCWSTR MakeIntreSource(int cn, int en);