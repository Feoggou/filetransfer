#pragma once

#ifndef GENERAL_H

#define WIN32_LEAN_AND_MEAN

#include "targetver.h"

#include <Windows.h>
#include <Objbase.h>
#include <strsafe.h>
#include <Commctrl.h>
#include <Shlwapi.h>
#include <Shellapi.h>
#include <Shobjidl.h>
#include <Shlobj.h>
#include <Commdlg.h>
#include <Windowsx.h>

#include <string>
using namespace std;

#pragma warning (disable: 4482)

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

#define theApp App::Get()
#define hMainWnd App::Get().GetHWND()

#include "String.h"
#include "Socket.h"
#include "DoubleList.h"
#include "File.h"
#include "LoadFile.h"
#include "SaveFile.h"

#define QWORD UINT64

#ifndef _DEBUG
#define _ASSERT(x) x
#define _ASSERTE(x) x
#endif

//DEBUG FUNCTIONS
#ifdef _DEBUG
#include <crtdbg.h>

#define new new(_NORMAL_BLOCK,  __FILE__, __LINE__)
#define malloc(size) _malloc_dbg(size, _NORMAL_BLOCK,  __FILE__, __LINE__)

#endif//_DEBUG


//used with dwDataTransfer: specifies that the application is (or should) send item(s)
#define DATATRANSF_SEND		1
//used with dwDataTransfer: specifies that the application is (or should) receive item(s)
#define DATATRANSF_RECV		2
//the maximum size of a DataBlock buffer.
#define BLOCKSIZE			0x28000//10240

//MESSAGES
//sent to the main window to enable or disable the control with the specified HWND
//WPARAM - HWND of the child
//LPARAM - 1: enable; 0: disable
#define WM_ENABLECHILD		WM_USER + 12

//updates the text of certain static controls.
//LPARAM: 0 - for send info; 1 - for send curfile; 
//        2 - for recv info; 3 - for recv curfile;
//WPARAM: text
#define WM_SETITEMTEXT		WM_USER + 11

//calls the CloseAll function
#define WM_CLOSECONNECTION			WM_USER + 3

//shows a messagebox (so that it is called by the UI thread)
//lParam = the text; wParam = the style.
#define WM_SHOWMESSAGEBOX			WM_USER + 5

//the ID of the system tray icon
#define SYSTRAY_ICON		1000

//DATA TYPES
//specifies what is being transferred: a file or a folder
enum ItemType{Folder, File};
//specifies how the program is connected, and if it is connected
enum Conn{NotConnected, ConnAsServer, ConnAsClient};

//CDoubleList-s are used with items of this type: they represent the items in the parent folder
struct FILE_ITEM
{
	//full name, beginning with the root.
	WCHAR* wsFullName;
	LONGLONG size;
	int type;
};

//a class for strings: used to prevent memory leaks in places where it is hard to remember what and where to delete
class WSTR
{
public:
	WCHAR* s;
	WSTR()
	{
		s = NULL;
	}

	~WSTR()
	{
		if (s)
		{
			delete[] s;
		}
	}
};

//VARIABLES
//specifies that the connection should terminate.
extern BOOL bOrderEnd;
//specifies how the program is connected: as a server or as a client.
extern Conn Connected;
//specifies if the application is sending data, receiving data, both or none.
extern DWORD dwDataTransfer;

//FUNCTIONS
//converts a size from LARGE_INTEGER to a user-friendly string.
extern inline void SizeLItoString(LARGE_INTEGER&liSize, WCHAR* wsSize);
//counts the digits of the specified integer number
extern inline int CountDigits(int nInteger);
//searches within the folder: all file information is stored in Items; total size is stored in liSize.
void SearchFolder(IShellFolder* pSearchFolder, CDoubleList<FILE_ITEM> &Items, LARGE_INTEGER& liSize);
//calculates the size of the specified file
inline BOOL CalcFileSize(const WCHAR* wsPath, LARGE_INTEGER& liSize);
//checks to see if the specified file/folder exists or not.
extern inline BOOL PathFileExistsEx(const WCHAR* wsPath);

//formats the time in hh:mm:ss format
extern inline void FormatTime(WCHAR wsTime[20], DWORD dTime);
//converts a double that represents the transfer speed into a user-friendly transfer speed.
extern inline void SpeedFtoString(float fSpeed, WCHAR* wsSpeed);

//called by the doublelist when an item must be removed
extern void __stdcall OnDestroyFileItemCoTask(FILE_ITEM& fitem);

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#endif//GENERAL_H