#include "General.h"
#include "Socket.h"

extern BOOL bOrderEnd;
extern HWND hDlgStatus;

Socket::Socket(void)
{
	m_bCreated = FALSE;
	m_wPort = 0;
}

Socket::~Socket(void)
{
}

int Socket::InitSockets(void)
{
	WSADATA wsaData;
	int Result = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (Result != 0)
	{
		return Result;
	}

	return 0;
}

int Socket::UninitSockets(void)
{
	if (WSACleanup() != 0)
	{
		return WSAGetLastError();
	}

	return 0;
}