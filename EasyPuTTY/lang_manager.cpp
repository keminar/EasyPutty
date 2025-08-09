#include "lang_manager.h"
#include <winnls.h>
#include <stdarg.h>

LanguageType g_currentLang = LANG_EN;
static HINSTANCE g_hInstance = NULL;
static wchar_t g_stringBuffer[1024] = { 0 };

// ����Ӧ��ʵ���������WinMain�е��ã�
void SetLangInstance(HINSTANCE hInstance) {
	g_hInstance = hInstance;
}

void InitLanguage() {
	wchar_t iniPath[MAX_PATH] = { 0 };
	wchar_t lang[10] = { 0 };

	// ֧������Ӣ��
	GetAppIni(iniPath, MAX_PATH);
	GetPrivateProfileStringW(SECTION_NAME, L"Lang", L"", lang, 10, iniPath);
	if (wcscmp(lang, L"en") == 0) {
		g_currentLang = LANG_EN;
		return;
	}

	// ��ȡϵͳĬ������
	LCID locale = GetUserDefaultLCID();
	switch (PRIMARYLANGID(LANGIDFROMLCID(locale))) {
	case LANG_CHINESE:
		g_currentLang = LANG_CN;
		break;
	default:
		g_currentLang = LANG_EN;
		break;
	}
}

const wchar_t* GetString(int stringId) {
	if (!g_hInstance) return L"";

	// ���ݵ�ǰ���Ի�ȡ��Ӧ������ ID
	WORD langId;
	if (g_currentLang == LANG_EN) {
		langId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
	}
	else {
		langId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
	}

	// 1. ���Լ���ָ�����Ե���Դ
	HRSRC hRes = FindResourceExW(
		g_hInstance,
		RT_STRING,
		MAKEINTRESOURCEW((stringId >> 4) + 1),
		langId
	);

	// 2. ���ָ��������Դ�����ڣ����Լ���Ĭ�����ԣ��������ģ�
	if (!hRes) {
		hRes = FindResourceExW(
			g_hInstance,
			RT_STRING,
			MAKEINTRESOURCEW((stringId >> 4) + 1),
			MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED) // Ĭ������
		);
	}

	// 3. ������Ҳ�����Դ�����ؿ��ַ���
	if (!hRes) return L"";

	// ����������Դ���߼�����֮ǰ��ͬ��
	HGLOBAL hMem = LoadResource(g_hInstance, hRes);
	if (!hMem) return L"";

	wchar_t* pStr = (wchar_t*)LockResource(hMem);
	if (!pStr) return L"";

	int index = stringId & 0x0F;
	for (int i = 0; i < index; i++) {
		pStr += *pStr + 1;
	}

	size_t strLen = *pStr;
	if (strLen >= sizeof(g_stringBuffer) / sizeof(wchar_t)) {
		strLen = (sizeof(g_stringBuffer) / sizeof(wchar_t)) - 1;
	}
	wcsncpy_s(g_stringBuffer, sizeof(g_stringBuffer) / sizeof(wchar_t), pStr + 1, strLen);
	g_stringBuffer[strLen] = L'\0';

	UnlockResource(hMem);
	FreeResource(hMem);

	return g_stringBuffer;
}

int FormatString(wchar_t* buffer, size_t bufferSize, int stringId, ...) {
	va_list args;
	va_start(args, stringId);

	const wchar_t* format = GetString(stringId);
	int result = vswprintf_s(buffer, bufferSize, format, args);

	va_end(args);
	return result;
}