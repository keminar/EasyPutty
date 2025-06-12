#pragma once

#include "common.h"
#include <wchar.h>
#include <stdio.h>

// ��ȫ�ضϿ��ַ��ַ��������ʡ�Ժ�
void TruncateString(const wchar_t* src, wchar_t* dest, size_t maxLength) {
	size_t srcLen = wcslen(src);

	// ���Դ�ַ�������С�ڵ�����󳤶ȣ�ֱ�Ӹ���
	if (srcLen <= maxLength) {
		wcscpy_s(dest, srcLen + 1, src);
		return;
	}

	// ����ض�λ�ã�����ʡ�ԺŵĿռ䣩
	size_t truncatePos = maxLength - 3;  // ��ȥ 3 Ϊʡ�Ժ������ռ�

	// ���ض�λ���Ƿ��ڴ�����м�
	if (truncatePos > 0 &&
		src[truncatePos - 1] >= 0xD800 && src[truncatePos - 1] <= 0xDBFF &&  // �ߴ�����
		src[truncatePos] >= 0xDC00 && src[truncatePos] <= 0xDFFF) {           // �ʹ�����
		truncatePos--;  // �����ض�λ���Ա����ƻ������
	}

	// ���ƽضϵ��ַ���
	wcsncpy_s(dest, maxLength + 1, src, truncatePos);

	// ���ʡ�Ժ�
	wcscat_s(dest, maxLength + 1, L"...");
}