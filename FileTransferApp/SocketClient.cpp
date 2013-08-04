#include "SocketClient.h"
#include "General.h"
#include "MainDlg.h"
#include "Debug.h"

#include "Application.h"

char* SocketClient::m_sServerIP = NULL;

SocketClient::SocketClient(void):Socket()
{
}


SocketClient::~SocketClient(void)
{
	Close();

	if (SocketClient::m_sServerIP)
	{
		delete[] SocketClient::m_sServerIP;
		SocketClient::m_sServerIP = NULL;
	}
}

int SocketClient::Create(void)
{
	//client
	m_Server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_Server == INVALID_SOCKET)
	{
		return WSAGetLastError();
	}

	int time = 5000;
	int nError = setsockopt(m_Server, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof(time));
	if (nError != 0)
	{
		return WSAGetLastError();
	}

	/*DWORD dwValue = 1;
	int nError = ioctlsocket(m_Server, FIONBIO, &dwValue);
	if (nError != 0)
	{
		return WSAGetLastError();
	}*/

	m_bCreated = TRUE;
	return 0;
}

int SocketClient::Connect(WORD nPort)
{
	if (bOrderEnd) return 0;

	m_wPort = nPort;

	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.S_un.S_addr = inet_addr(m_sServerIP);
	ServerAddr.sin_port = htons(nPort);

	/*DWORD dwValue = 0;
	int nError = ioctlsocket(m_Server, FIONBIO, &dwValue);
	if (nError != 0)
	{
		return WSAGetLastError();
	}*/

	//client
	if (0 != connect(m_Server, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
	{
		return WSAGetLastError();
	}

	/*dwValue = 1;
	nError = ioctlsocket(m_Server, FIONBIO, &dwValue);
	if (nError != 0)
	{
		return WSAGetLastError();
	}*/

	return 0;
}

int SocketClient::Send(void* buffer, int len, int& dSentRec)
{
	if (bOrderEnd) 
	{
		dSentRec = SOCKET_ERROR;
		return 0;
	}

	int i = 0;
	do
	{
		while (Connected == Conn::NotConnected)
		{
			if (bOrderEnd) return false;
			Sleep(500);
		}

		int nResult = send(m_Server, (i + (char*)buffer), len, 0);
		dSentRec = nResult;

		if (SOCKET_ERROR == nResult)
		{
			Sleep(200);
			nResult = WSAGetLastError();
			/*switch (nResult)
			{
			case WSAENETDOWN:
			case WSAENETRESET:
			case WSAEHOSTUNREACH:
			case WSAECONNABORTED:
			case WSAECONNRESET:
			case WSAETIMEDOUT:
				if (Reconnect())
				{
					break;
				}

				else
				{
					return 0;
				}

			default:
				DisplayError(nResult);
				dSentRec = -1;
				return 0;
			}*/
		}

		else if (dSentRec == len)
		{
			break;
		}

		else if (dSentRec < len)
		{
			//If this case is indeed possible, let's do the things as I think it is correct
			i = dSentRec;
			len -= i;
		}
	} while (1/*dSentRec < len*/);

	return 0;
}

int SocketClient::Receive(void* buffer, int len, int& dSentRec)
{
	if (bOrderEnd) 
	{
		dSentRec = SOCKET_ERROR;
		return 0;
	}

	int i = 0, nResult, nCountTries = 0;
	do
	{
		while (Connected == Conn::NotConnected)
		{
			if (bOrderEnd) return false;
			Sleep(500);
		}

		nResult = recv(m_Server, (i + (char*)buffer), len, 0);
		dSentRec = nResult;

		//if an error has occured...
		if (SOCKET_ERROR == nResult)
		{
			nResult = WSAGetLastError();
			switch (nResult)
			{
			case WSAETIMEDOUT:
			case WSAEWOULDBLOCK:
			{
				if (Connected != Conn::NotConnected)
				{
					Reconnect();
				}

				return 0;
			}
			break;

			case WSAENETDOWN:
			case WSAENETRESET:
			case WSAEHOSTUNREACH:
			case WSAECONNABORTED:
			case WSAECONNRESET:
			case WSAEINTR:
			case WSAENOTSOCK:
				if (Connected != Conn::NotConnected)
				{
					Reconnect();
				}

				return 0;
				break;

			default:
				DisplayError(nResult);
				dSentRec = -1;
				return 0;
			}
		}

		else if (dSentRec == 0)
		{
			//connection gracefully closed
			return 0;
		}

		else if (dSentRec == len)
		{
			break;
		}

		else if (dSentRec < len)
		{
			//If this case is indeed possible, let's do the things as I think it is correct
			i = dSentRec;
			len -= i;
		}
	} while (1/*dSentRec < len*/);

	return 0;
}

int SocketClient::Close(void)
{
	if (!m_bCreated) return 0;

	shutdown(m_Server, SD_BOTH);

	if (m_Server && 0 != closesocket(m_Server)) 
	{
		return WSAGetLastError(); 
	}
	else m_Server = 0;

	m_bCreated = FALSE;

	return 0;
}

#include "Recv.h"
#include "Send.h"

BOOL SocketClient::Reconnect()
{
	return false;
	//TODO: fix later!

//	if (bOrderEnd) return FALSE;
//	int nError;
//
//	Connected = Conn::NotConnected;
//
//	int port = 14147;
//	SocketClient* pClient = (SocketClient*)theApp->GetReceiveSocket();
//	if (!pClient) {
//		pClient = (SocketClient*)theApp->GetSendSocket();
//		ASSERT(pClient);
//
//		port = 14148;
//	}
//
//	PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonBrowse, 0);
//	PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonSend, 0);
//	SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connection to the server has been lost. Trying to reconnect.");
//
//	nError = pClient->Close();
//	if (nError)
//	{
//		DisplayError(nError);
//		return false;
//	}
//
//	nError = pClient->Create();
//	if (nError)
//	{
//		DisplayError(nError);
//		return false;
//	}
//
//	//Connect
//try_again:
//	nError = pClient->Connect(port);
//	if (nError && !bOrderEnd)
//	{
//		Sleep(200);
//		goto try_again;
//	}; 
//
//	if (bOrderEnd) return false;
//
//	DWORD dwValue = 1;
//	nError = ioctlsocket(pClient->m_Server, FIONBIO, &dwValue);
//	if (nError != 0)
//	{
//		return false;//WSAGetLastError();
//	}
//
//	char buffer[4];
//	while (recv(pClient->m_Server, buffer, 4, 0) > 0);
//
//	dwValue = 0;
//	nError = ioctlsocket(pClient->m_Server, FIONBIO, &dwValue);
//	if (nError != 0)
//	{
//		return false;//WSAGetLastError();
//	}
//
//	Connected = Conn::ConnAsClient;
//
//	//after the connection is succesful:
//	SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connection to the server has been established.");
//	PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonBrowse, 1);
//	PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonSend, 1);
//	
//	return true;
}