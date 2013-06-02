#include "SocketServer.h"
#include "General.h"
#include "MainDlg.h"

#include "Application.h"

CSocketServer::CSocketServer(void):Socket()
{
}


CSocketServer::~CSocketServer(void)
{
	Close();
}

int CSocketServer::Create(WORD nPort)
{
	m_wPort = nPort;

	SOCKADDR_IN InternetAddr;
	InternetAddr.sin_family = AF_INET;
	InternetAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	InternetAddr.sin_port = htons(nPort);

	//server
	m_Server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_Server == INVALID_SOCKET)
	{
		return WSAGetLastError();
	}

	if (SOCKET_ERROR ==  bind(m_Server, (sockaddr*)&InternetAddr, sizeof(InternetAddr)))
	{
		return WSAGetLastError();
	}

	int time = 5000;
	int nError = setsockopt(m_Server, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof(time));
	if (nError != 0)
	{
		return WSAGetLastError();
	}

	m_bCreated = TRUE;

	return 0;
}

int CSocketServer::Listen()
{
	if (SOCKET_ERROR == listen(m_Server, 1))
	{
		return WSAGetLastError();
	}

	return 0;
}

int CSocketServer::Accept(void)
{
	SOCKADDR_IN ClientAddr;
	int dSize = sizeof(SOCKADDR_IN);
	m_Connection = accept(m_Server, (SOCKADDR*)&ClientAddr, &dSize);
	if (m_Connection == INVALID_SOCKET)
	{
		return WSAGetLastError();
	}

	int time = 5000;
	int nError = setsockopt(m_Connection, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof(time));
	if (nError != 0)
	{
		return WSAGetLastError();
	}

	/*DWORD dwValue = 1;
	int nError = ioctlsocket(m_Connection, FIONBIO, &dwValue);
	if (nError != 0)
	{
		return WSAGetLastError();
	}*/

	return 0;
}

int CSocketServer::Send(void* buffer, int len, int& dSentRec)
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

		int nResult = send(m_Connection, (i + (char*)buffer), len, 0);
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

int CSocketServer::Receive(void* buffer, int len, int& dSentRec)
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
		//receive data
		nResult = recv(m_Connection, (i + (char*)buffer), len, 0);
		dSentRec = nResult;

		//if an error has occured...
		if (SOCKET_ERROR == nResult)
		{
			nResult = WSAGetLastError();
			switch (nResult)
			{
			case WSAETIMEDOUT:
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

int CSocketServer::Close(void)
{
	if (!m_bCreated) return 0;

	shutdown(m_Server, SD_BOTH);
	if(m_Connection && 0 != closesocket(m_Connection))
	{
		closesocket(m_Server);
		return WSAGetLastError(); 
	}
	else m_Connection = 0;

	if (m_Server && 0 != closesocket(m_Server)) 
	{
		return WSAGetLastError(); 
	}
	else m_Server = 0;

	m_bCreated = FALSE;

	return 0;
}

#include "Send.h"
#include "Recv.h"

BOOL CSocketServer::Reconnect()
{
	if (bOrderEnd) return FALSE;

	Connected = Conn::NotConnected;
	
	CSocketServer* pRecvServer = (CSocketServer*)Recv::pSocket;
	CSocketServer* pSendServer = (CSocketServer*)Send::pSocket;

	PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonBrowse, 0);
	PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonSend, 0);
	SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connection to the client has been lost. Trying to reconnect.");

	int nError = pSendServer->Close();
	if (nError)
	{
		DisplayError(nError);
		return false;
	}

	nError = pRecvServer->Close();
	if (nError)
	{
		DisplayError(nError);
		return false;
	}

	nError = pRecvServer->Create(14147);
	if (nError)
	{
		DisplayError(nError);
		return false;
	}

	nError = pSendServer->Create(14148);
	if (nError)
	{
		DisplayError(nError);
		return false;
	}

	//Listen
	nError = pRecvServer->Listen();
	if (nError)
	{
		DisplayError(nError);
		return false;
	}

	nError = pSendServer->Listen();
	if (nError)
	{
		DisplayError(nError);
		return false;
	}

	//Accept
	if (!bOrderEnd)
	{
		nError = pRecvServer->Accept();//14147
		if (nError && !bOrderEnd)
		{
			DisplayError(nError);
			return false;
		}
	}

	nError = pSendServer->Accept();//14148
	if (nError && !bOrderEnd)
	{
		DisplayError(nError);
		return false;
	}

	if (bOrderEnd) return false;

	DWORD dwValue = 1;
	nError = ioctlsocket(pRecvServer->m_Connection, FIONBIO, &dwValue);
	if (nError != 0)
	{
		return false;//WSAGetLastError();
	}

	nError = ioctlsocket(pSendServer->m_Connection, FIONBIO, &dwValue);
	if (nError != 0)
	{
		return false;//WSAGetLastError();
	}

	char buffer[4];
	while (recv(pRecvServer->m_Connection, buffer, 4, 0) > 0);
	while (recv(pSendServer->m_Connection, buffer, 4, 0) > 0);

	dwValue = 0;
	nError = ioctlsocket(pRecvServer->m_Connection, FIONBIO, &dwValue);
	if (nError != 0)
	{
		return false;//WSAGetLastError();
	}

	nError = ioctlsocket(pSendServer->m_Connection, FIONBIO, &dwValue);
	if (nError != 0)
	{
		return false;//WSAGetLastError();
	}

	Connected = Conn::ConnAsServer;

	//after the connection is successful:
	SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connection to the client has been established.");
	PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonBrowse, 1);
	PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonSend, 1);
	
	return true;
}