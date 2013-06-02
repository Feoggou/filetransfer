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

#pragma warning (disable: 4482)

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