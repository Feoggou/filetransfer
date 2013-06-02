#include "General.h"
#include "SamSocket.h"

extern BOOL bOrderEnd;
extern HWND hDlgStatus;

CSamSocket::CSamSocket(void)
{
	m_bCreated = FALSE;
	m_wPort = 0;
}

CSamSocket::~CSamSocket(void)
{
}

int CSamSocket::InitSockets(void)
{
	WSADATA wsaData;
	int Result = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (Result != 0)
	{
		return Result;
	}

	return 0;
}

int CSamSocket::UninitSockets(void)
{
	if (WSACleanup() != 0)
	{
		return WSAGetLastError();
	}

	return 0;
}