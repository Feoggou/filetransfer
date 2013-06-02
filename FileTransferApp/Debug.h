#pragma once

#include <Windows.h>
#include <strsafe.h>

//display an error message
inline BOOL DisplayError(DWORD dwError = 0)
{
	if (dwError == 0) dwError = GetLastError();

	if (dwError)
	{
		WCHAR wsError[500];
		WCHAR* wsDestEnd;
		StringCchPrintfExW(wsError, 500, &wsDestEnd, NULL, 0, L"Error 0x%08X: ", dwError);

		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError, 0, wsDestEnd, 500, 0);
		
		MessageBox(0, wsError, 0, MB_ICONERROR);

		return TRUE;
	}
	return FALSE;
}

//#include "String.h"
//#include "Socket.h"
//#include "DoubleList.h"
//#include "File.h"
//#include "SourceFile.h"
//#include "DestFile.h"

#define QWORD UINT64

#ifndef _DEBUG
#define _ASSERT(x)
#define _ASSERTE(x)
#endif

#define FTA_TEST

#ifdef _DEBUG
#define ASSERT(x) _ASSERT(x)
#define ASSERTE(x) _ASSERTE(x)
#elif defined (FTA_TEST)
#define ASSERT(x) _ASSERT(x)
#define ASSERTE(x) _ASSERTE(x)
#else
#define ASSERT(x)
#define ASSERTE(x) 
#endif

//DEBUG FUNCTIONS
#ifdef _DEBUG
#include <crtdbg.h>

#define new new(_NORMAL_BLOCK,  __FILE__, __LINE__)
#define malloc(size) _malloc_dbg(size, _NORMAL_BLOCK,  __FILE__, __LINE__)

#endif//_DEBUG