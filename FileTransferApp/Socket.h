#pragma once
#include <winsock2.h>

class CSamSocket abstract
{
protected:
	//specifies whether the socket is connected or not.
	BOOL m_bCreated;

	//the communication port
	WORD m_wPort;

public:
	//the socket: if this is the server, it is the socket of this computer;
	//if this is the client, it is the socket of the other computer (the server).
	SOCKET m_Server;

	//the constructor
	CSamSocket(void);
	//the destructor
	virtual ~CSamSocket(void);
	
	//initializes the sockets: this allows the sockets to be created
	static int InitSockets(void);
	//uninitializes the sockets: this must be done at the end of the application
	static int UninitSockets(void);

	//it closes the socket(s)/connection
	virtual int Close(void) = 0;

	//it attempts reconnection, in case of failure.
	virtual BOOL Reconnect() = 0;

	//sends data
	virtual int Send(void* buffer, int len, int& dSentRec) = 0;
	//receives data
	virtual int Receive(void* buffer, int len, int& dSentRec) = 0;
};
