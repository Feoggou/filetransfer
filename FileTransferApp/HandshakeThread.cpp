#include "HandshakeThread.h"
#include "Application.h"

#include "MainDlg.h"
#include "String.h"


HandshakeThread::HandshakeThread(Socket* p)
	: m_pSocket(p)
{
	ASSERT(m_pSocket);
}

HandshakeThread::~HandshakeThread()
{
}

void HandshakeThread::OnStart()
{
	int dSentRec;
	BYTE aux;
	BYTE dIsReady = 0;

	do
	{
		if (bOrderEnd) return /*false*/;
		Sleep(500);

		//sending the news: ready or not ready to send the file
		m_pSocket->Send(&dIsReady, sizeof(dIsReady), dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return /*false*/;
			continue;
		}

		//receiving confirmation: this data does not matter what it is
		m_pSocket->Receive(&aux, sizeof(aux), dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return /*false*/;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return /*false*/;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

	}while (!dIsReady);

	return /*true*/;
}