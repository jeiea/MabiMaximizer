// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WINVER 0x0501
#define _WIN32_WINNT WINVER
#define _WIN32_IE 0x0600
#include "targetver.h"

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <tchar.h>

// TODO: reference additional headers your program requires here
#include <Psapi.h>
#include <Shlwapi.h>

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Shlwapi.lib")