#pragma once

#include "common.h"
#include <wchar.h>
#include <stdio.h>

// 安全截断宽字符字符串并添加省略号
void TruncateString(const wchar_t* src, wchar_t* dest, size_t maxLength) {
	size_t srcLen = wcslen(src);

	// 如果源字符串长度小于等于最大长度，直接复制
	if (srcLen <= maxLength) {
		wcscpy_s(dest, srcLen + 1, src);
		return;
	}

	// 计算截断位置（留出省略号的空间）
	size_t truncatePos = maxLength - 3;  // 减去 3 为省略号留出空间

	// 检查截断位置是否在代理对中间
	if (truncatePos > 0 &&
		src[truncatePos - 1] >= 0xD800 && src[truncatePos - 1] <= 0xDBFF &&  // 高代理项
		src[truncatePos] >= 0xDC00 && src[truncatePos] <= 0xDFFF) {           // 低代理项
		truncatePos--;  // 调整截断位置以避免破坏代理对
	}

	// 复制截断的字符串
	wcsncpy_s(dest, maxLength + 1, src, truncatePos);

	// 添加省略号
	wcscat_s(dest, maxLength + 1, L"...");
}