#include "HandShakeThread.h"

//
//HandShakeThread::HandShakeThread(void)
//{
//}
//
//
//HandShakeThread::~HandShakeThread(void)
//{
//}

void HandShakeThread::Run()
{
	int dSentRec;
	BYTE aux;

	do
	{
		if (bOrderEnd) return false;
		Sleep(500);

		//sending the news: ready or not ready to send the file
		pSocket->Send(&dIsReady, sizeof(dIsReady), dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

		//receiving confirmation: this data does not matter what it is
		pSocket->Receive(&aux, sizeof(aux), dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(hMainWnd, WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

	}while (!dIsReady);

	return true;
}