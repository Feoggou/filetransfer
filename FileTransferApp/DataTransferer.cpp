#include "DataTransferer.h"
#include "General.h"
#include "Debug.h"
#include "CRC.h"
#include "Application.h"
#include "Socket.h"
#include "MainDlg.h"


DataTransferer::DataTransferer(Socket* p)
	: m_pSocket(p)
{
}

DataTransferer::DataTransferer()
	: m_pSocket(nullptr)
{
}



DataTransferer::~DataTransferer(void)
{
}

void DataTransferer::SetSocket(Socket* pSocket)
{
	m_pSocket = pSocket;
}


bool DataTransferer::SendData(void* Buffer, int dSize)
{
	ASSERT(m_pSocket);

	_ASSERTE(Buffer);
	int dSentRec;
	CRC crc;
	DWORD dwIsOK = 0;

	do
	{
		//calculate the checksum
		crc = CRCCalc((BYTE*)Buffer, dSize);

		//send the checksum
		m_pSocket->Send(&crc, sizeof(crc), dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

		//send data
		m_pSocket->Send(Buffer, dSize, dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

		//receive validation
		m_pSocket->Receive(&dwIsOK, sizeof(dwIsOK), dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

		//if validation failed try again!
	} while (dwIsOK != 1);

	return true;
}

bool DataTransferer::ReceiveData(void* Buffer, int dSize)
{
	ASSERT(m_pSocket);

	_ASSERTE(Buffer);
	int dSentRec;
	CRC crc1, crc2;
	DWORD dwIsOK = 0;

	do
	{
		//receive the checksum
		m_pSocket->Receive(&crc1, sizeof(crc1), dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

		//receive data
		m_pSocket->Receive(Buffer, dSize, dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

		//calculate the checksum
		crc2 = CRCCalc((BYTE*)Buffer, dSize);

		if (crc1 == crc2)
		{
			dwIsOK = 1;
			m_pSocket->Send(&dwIsOK, sizeof(dwIsOK), dSentRec);
		}
		else
		{
			dwIsOK = 0;
			m_pSocket->Send(&dwIsOK, sizeof(dwIsOK), dSentRec);
		}

		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

		//if validation failed try again!
	} while (dwIsOK != 1);

	return true;
}

bool DataTransferer::SendDataSimple(void* Buffer, int dSize)
{
	ASSERT(m_pSocket);

	_ASSERTE(Buffer);
	int dSentRec;

	//send data
try_again:
	m_pSocket->Send(Buffer, dSize, dSentRec);
	if (dSentRec == -1)
	{
		if (bOrderEnd) return false;
		goto try_again;
	}

	return true;
}

bool DataTransferer::ReceiveDataSimple(void* Buffer, int dSize)
{
	ASSERT(m_pSocket);

	_ASSERTE(Buffer);
	int dSentRec;

try_again:
	//receive data
	m_pSocket->Receive(Buffer, dSize, dSentRec);
	if (dSentRec <= 0)
	{
		if (bOrderEnd) return false;
		if (0 == dSentRec)
		{
			SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
			PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
			return false;
		}

		else if (dSentRec == -1)
		{
			goto try_again;
		}
	}

	return true;
}

bool DataTransferer::SendDataShort(void* Buffer, int dSize)
{
	ASSERT(m_pSocket);

	_ASSERTE(Buffer);
	int dSentRec;
	DWORD dwIsOK = 0;

	do
	{
		//send data
		m_pSocket->Send(Buffer, dSize, dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

		//receive data
		BYTE* buf2 = new BYTE[dSize];
		m_pSocket->Receive(buf2, dSize, dSentRec);
		if (dSentRec <= 0)
		{
			delete[] buf2;
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

		//comparing data1 with data2
		if (0 == memcmp(Buffer, buf2, dSize))
		{
			dwIsOK = 1;
		}
		else dwIsOK = 0;

		delete[] buf2;

		//sending validation
		m_pSocket->Send(&dwIsOK, sizeof(dwIsOK), dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

		//if validation failed try again!
	} while (dwIsOK != 1);

	return true;
}

bool DataTransferer::ReceiveDataShort(void* Buffer, int dSize)
{
	ASSERT(m_pSocket);

	_ASSERTE(Buffer);
	int dSentRec;
	DWORD dwIsOK = 0;

	do
	{
		//receive data
		m_pSocket->Receive(Buffer, dSize, dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

		//sending data
		m_pSocket->Send(Buffer, dSize, dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

		//receiving validation
		m_pSocket->Receive(&dwIsOK, sizeof(dwIsOK), dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

		//if validation failed try again!
	} while (dwIsOK != 1);

	return true;
}

bool DataTransferer::WaitForDataReceive()
{
	ASSERT(m_pSocket);

	int dSentRec;
	BYTE dIsReady = 0;

	do
	{
		if (bOrderEnd) return false;
		Sleep(500);

		//receiving the news: whether a file will follow or not
		m_pSocket->Receive(&dIsReady, sizeof(dIsReady), dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

		//sending the acknowledgement: we have read the message.
		//this value is not used by the other computer.
		m_pSocket->Send(&dIsReady, sizeof(dIsReady), dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

	}while (!dIsReady);

	return true;
}

BYTE dIsReady = 0;
//the Handshake thread
DWORD DataTransferer::HandShake(void* pv_socket)
{
	Socket* pSocket = (Socket*)pv_socket;
	ASSERT(pSocket);

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
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
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

bool DataTransferer::WaitForDataSend()
{
	int dSentRec;
	BYTE aux;
	BYTE dIsReady = 0;

	do
	{
		if (bOrderEnd) return false;
		Sleep(500);
		dIsReady = dwDataTransfer & DATATRANSF_SEND;

		//sending the news: ready or not ready to send the file
		m_pSocket->Send(&dIsReady, sizeof(dIsReady), dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

		//receiving confirmation: this data does not matter what it is
		m_pSocket->Receive(&aux, sizeof(aux), dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
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