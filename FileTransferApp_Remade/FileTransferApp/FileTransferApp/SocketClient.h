#pragma once

#ifndef SOCKETCLIENT_H
#define SOCKETCLIENT_H

#include "samsocket.h"

class SocketClient : public Socket
{
	//PRIVATE DATA
public:
	//if this application is acting as a client, this is the IPv4 of the server.
	static char* m_sServerIP;

public:
	SocketClient(void);
	~SocketClient(void);

	//creates the connection in which this computer is the client
	int Create(void);
	//if this is the client, it connects it to the server
	int Connect(WORD nPort);
	//it closes the socket(s)/connection
	int Close(void);
	//it attempts reconnection, in case of failure.
	BOOL Reconnect();

	//sends data
	int Send(void* buffer, int len, int& dSentRec);
	//receives data
	int Receive(void* buffer, int len, int& dSentRec);
};

#endif//SOCKETCLIENT_H