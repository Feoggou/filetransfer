#include <math.h>

#include "General.h"
#include "MainDlg.h"
#include "resource.h"
#include "Recv.h"
#include "Send.h"

#include "String.h"
#include "CRC.h"
#include "App.h"
#include "Exceptions.h"

//the variable that specifies if the connection and all that goes with it should end or not
BOOL bOrderEnd = false;
//this specifies how the program is currently connected (as a server, as a client, or not connected)
Conn Connected = Conn::NotConnected;
//this specifies the status of the transfer: if file(s) are being sent, are being retrieved or both
DWORD dwDataTransfer = 0;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	//initialize COM
	CoInitialize(0);

	theApp.SetInstance(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	//initialize the common controls
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	//initialize the sockets
	int nError = Socket::InitSockets();
	if (nError)
	{
		DisplayError(nError);
		return -1;
	}

	crcInit();

	//create the main dialogbox
	try
	{
		//ThrowWin(309); //(for test)
		theApp.CreateMainDlg();
		//theApp.CreateMainDlg();//test: written twice
	}
	catch (Exceptions::Exception& e)
	{
		Exceptions::ReportError(e.what(), "A critical error has occured! The program will now exit.");
	}

	//uninitialize the sockets
	nError = Socket::UninitSockets();
	if (nError)
	{
		DisplayError(nError);
	}

	//uninitialize COM
	CoUninitialize();
	return 0;
}