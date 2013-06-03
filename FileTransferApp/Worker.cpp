#include "Worker.h"
#include "Debug.h"
#include "SocketClient.h"
#include "MainDlg.h"
#include "Application.h"


Worker::Worker(void)
	: m_dataTransferer(),
	m_pSocket(NULL)
{
}


Worker::~Worker(void)
{
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

void Worker::StartConnThread()
{
	m_connectionThread.Start(ConnThreadProc, this);
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

DWORD Worker::ConnThreadProc(void* p)
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
	nError = pSocketClient->Connect(14148);
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