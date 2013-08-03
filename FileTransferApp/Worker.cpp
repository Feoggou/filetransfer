#include "Worker.h"
#include "Debug.h"
#include "SocketClient.h"
#include "MainDlg.h"
#include "Application.h"
#include "DestFile.h"
#include "SourceFile.h"
#include "SocketServer.h"

Worker::Worker(bool is_receive, HWND hProgressBar)
	: m_dataTransferer(),
	m_pSocket(nullptr),
	m_pFile(nullptr),
	m_transferProgress(!is_receive, hProgressBar)
{
	if (is_receive) {
		m_pFile = new DestFile;
	} else {
		m_pFile = new SourceFile;
	}
}


Worker::~Worker(void)
{
	delete m_pFile;
	m_pFile = nullptr;
}



void Worker::StopThreads()
{
	if (m_connectionThread.IsRunning())
	{
		m_connectionThread.WaitAndClose();
	}

	//first the Recv::hThread
	if (m_transferThread.IsRunning())
	{
		m_transferThread.WaitAsyncAndClose();
	}
}

void Worker::StartConnThread(bool server)
{
	m_connectionThread.Start(server? ServerConnThreadProc : ClientConnThreadProc, this);
}

void Worker::CloseSocket()
{
	if (m_pSocket)
	{
		int nError = m_pSocket->Close();
		if (nError)
		{
			DisplayError(nError);

			delete m_pSocket;
			m_pSocket = NULL;
			PostQuitMessage(-1);
		}

		else
		{
			delete m_pSocket;
			m_pSocket = nullptr;
		}
	}
}

void Worker::CloseFile()
{
	if (m_pFile) {
		m_pFile->Close();
	}
}

DWORD Worker::ClientConnThreadProc(void* p)
{
	Worker* pThis = (Worker*)p;

	bOrderEnd = false;
	int nError;

	pThis->m_pSocket = new SocketClient();
	SocketClient* pSocketClient = (SocketClient*)pThis->m_pSocket;

	pThis->m_dataTransferer.SetSocket(pThis->m_pSocket);

	nError = pSocketClient->Create();
	if (nError)
	{
		DisplayError(nError);
		bOrderEnd = 1;
		goto final;
	}

	SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connecting to the server...");

try_again:
	nError = pSocketClient->Connect(14147);//was: 14148
	if (nError && !bOrderEnd)
	{
		if (nError == WSAECONNREFUSED) {Sleep(200); goto try_again;}
		if (nError != WSAETIMEDOUT)
		{
			DisplayError(nError);
			bOrderEnd = 1;
			goto final;
		}
		else
		{
			SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connection time-out. Trying again...");
				goto try_again;
		}

	};

final:
	pThis->m_connectionThread.Close();
	Connected = Conn::ConnAsClient;

	if (!bOrderEnd)
	{
		pThis->m_transferThread.Start(Recv::ThreadProc, pThis);

		//after the connection is succesful:
		SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connection to the server has been established.");
		PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonBrowse, 1);
	}
	else PostMessage(theApp->GetMainWindow(), WM_CLOSE, 0, 0);

	return 0;
}

DWORD Worker::ServerConnThreadProc(void* p)
{
	Send* pThis = (Send*)p;

	bOrderEnd = false;
	int nError;

	pThis->m_pSocket = new SocketServer();
//	Recv::pSocket = new SocketServer();
//	SocketServer* pRecvServer = (SocketServer*)Recv::pSocket;
	SocketServer* pSendServer = (SocketServer*)pThis->m_pSocket;
	pThis->m_dataTransferer.SetSocket(pThis->m_pSocket);

	/*nError = pRecvServer->Create(14148);
	if (nError)
	{
		DisplayError(nError);
		goto final;
	}*/

	nError = pSendServer->Create(14147);
	if (nError)
	{
		DisplayError(nError);
		goto final;
	}

	////Listen
	//nError = pRecvServer->Listen();
	//if (nError)
	//{
	//	DisplayError(nError);
	//	goto final;
	//}

	nError = pSendServer->Listen();
	if (nError)
	{
		DisplayError(nError);
		goto final;
	}

	SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The Server is turned on and waiting for the client to connect.");

	//Accept
	/*if (!bOrderEnd)
	{
		nError = pRecvServer->Accept();
		if (nError && !bOrderEnd)
		{
			DisplayError(nError);
			goto final;
		}
	}*/

	nError = pSendServer->Accept();
	if (nError && !bOrderEnd)
	{
		DisplayError(nError);
		goto final;
	}

final:
	pThis->m_connectionThread.Close();
	Connected = Conn::ConnAsServer;
	
	if (!bOrderEnd)
	{
		//TODO: why was Recv::thread started here?
		//Recv::thread.Start(Recv::ThreadProc/*, theApp->GetMainWindow()*/);
		pThis->m_transferThread.Start(Send::ThreadProc, pThis/*theApp->GetMainWindow()*/);
		//Send::hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Send::ThreadProc, theApp->GetMainWindow(), 0, 0);

		//after the connection is successful:
		SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connection to the client has been established.");
		PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonBrowse, 1);
	}
	
	return 0;
}