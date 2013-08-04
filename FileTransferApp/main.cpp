#include <math.h>
#include <exception>

#include "General.h"
#include "Application.h"

#include "MainDlg.h"
#include "resource.h"
#include "Recv.h"
#include "Send.h"

#include "String.h"

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

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	MessageBox(0, L"The app is during refactoring, and is not functional yet. If you want something, check out the first commit(s)!", L"Hard luck!", 0);

	try {
		Application app(hInstance);

		app.ShowMainDialog();

	} catch (std::exception& e) {
		//TODO
		MessageBoxA(0, e.what(), "Fatal error!", MB_ICONERROR);
	}

	return 0;
}