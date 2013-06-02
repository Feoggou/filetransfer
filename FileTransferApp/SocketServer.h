#pragma once

#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include "Socket.h"

class CSocketServer : public Socket
{
	//PRIVATE DATA
private:
	//if this is the server, this is the socket of the client (other computer)
	SOCKET m_Connection;

public:
	CSocketServer(void);
	~CSocketServer(void);

	//creates the connection in which this computer is the server
	int Create(WORD nPort);
	//listens for incomming connections
	int Listen();
	//accepts an incomming connection
	int Accept(void);
	//it closes the socket(s)/connection
	int Close(void);
	//it attempts reconnection, in case of failure.
	BOOL Reconnect();

	//sends data
	int Send(void* buffer, int len, int& dSentRec);
	//receives data
	int Receive(void* buffer, int len, int& dSentRec);
};

#endif//SOCKETSERVER_H