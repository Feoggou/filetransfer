#include "Application.h"
#include "Exception.h"
#include "MainDlg.h"

#include "CRC.h"

Application* Application::s_pInst = nullptr;
HINSTANCE Application::s_hInst = NULL;

Application::Application(HINSTANCE hInst)
{
	s_hInst = hInst;

	_ASSERT(!s_pInst);

	s_pInst = this;

	CoInitialize(0);

	//initialize the common controls
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	//initialize the sockets
	int nError = CSamSocket::InitSockets();
	if (nError)
	{
		THROW(WindowsException, nError);

		DisplayError(nError);
		//return -1;
	}

	crcInit();
}


Application::~Application(void)
{
	//uninitialize the sockets
	int nError = CSamSocket::UninitSockets();
	if (nError)
	{
		DisplayError(nError);
	}

	CoUninitialize();
}

void Application::ShowMainDialog()
{
	m_pMainDlg->DoModal();
}

HWND Application::GetMainWindow()
{
	return m_pMainDlg->GetHandle();
}