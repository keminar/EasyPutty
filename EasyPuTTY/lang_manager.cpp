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

const wchar_t* GetStringBase(int stringId, bool isMultiFilter) {
	if (!g_hInstance) return L"";

	ZeroMemory(g_stringBuffer, sizeof(g_stringBuffer));
	// ���ݵ�ǰ���Ի�ȡ��Ӧ������ ID
	WORD langId;
	if (g_currentLang == LANG_CN) {
		langId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
	}
	else {
		langId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
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
	if (!pStr) {
		FreeResource(hMem);
		return L"";
	}

	int index = stringId & 0x0F;
	for (int i = 0; i < index; i++) {
		pStr += *pStr + 1;
	}

	size_t bufferSize = sizeof(g_stringBuffer) / sizeof(wchar_t);
	if (isMultiFilter) {
		// --------------------------
		// �ؼ�������������ַ����������\0�ָ�����
		// --------------------------
		wchar_t* filterSource = pStr + 1;          // �����������ֽڡ���ָ�����������
		size_t filterTotalLen = *pStr;             // ����Դ��ȡ�������ܳ��ȣ�������\0��

		// ��֤��Դ��ʽ�����������뺬����1��\0���ָ�����ʾ���ơ��͡�����ģʽ����
		bool hasSeparator = false;
		for (size_t i = 0; i < filterTotalLen; i++) {
			if (filterSource[i] == L'\0') {
				hasSeparator = true;
				break;
			}
		}
		if (!hasSeparator) {
			// ��Դ��ʽ�����ֶ�������������������������
			wcscpy_s(g_stringBuffer, L"All files(*.*)\0*.*\0\0");
			UnlockResource(hMem);
			FreeResource(hMem);
			return g_stringBuffer;
		}

		// ����������������������\0�ָ�����������ض�
		size_t copyLen = min(filterTotalLen, bufferSize - 2); // ��2��λ��ȷ��˫\0��ֹ
		memcpy(g_stringBuffer, filterSource, copyLen * sizeof(wchar_t));

		// ǿ�Ʋ���˫\0����������׼������־��ȷ��GetOpenFileNameʶ��
		g_stringBuffer[copyLen] = L'\0';
		g_stringBuffer[copyLen + 1] = L'\0';
	}
	else {
		// ������ͨ�ַ���
		size_t strLen = *pStr;
		if (strLen >= bufferSize) {
			strLen = bufferSize - 1;
		}
		wcsncpy_s(g_stringBuffer, bufferSize, pStr + 1, strLen);
		g_stringBuffer[strLen] = L'\0';
	}

	UnlockResource(hMem);
	FreeResource(hMem);

	return g_stringBuffer;
}

const wchar_t* GetString(int stringId) {
	return GetStringBase(stringId, FALSE);
}

const wchar_t* GetStringBrowser(int stringId) {
	return GetStringBase(stringId, TRUE);
}

int FormatString(wchar_t* buffer, size_t bufferSize, int stringId, ...) {
	va_list args;
	va_start(args, stringId);

	const wchar_t* format = GetString(stringId);
	int result = vswprintf_s(buffer, bufferSize, format, args);

	va_end(args);
	return result;
}

LPCWSTR MakeIntreSource(int cn, int en) {
	if (g_currentLang == LANG_CN) {
		return MAKEINTRESOURCEW(cn);
	}
	else {
		return MAKEINTRESOURCEW(en);
	}
}