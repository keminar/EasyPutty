// header.h: 标准系统包含文件的包含文件，
// 或特定于项目的包含文件
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>
#include <windowsx.h>
// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>
#include <wchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <string.h>
#include <tlhelp32.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <shellapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")