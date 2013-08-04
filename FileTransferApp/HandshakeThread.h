#pragma once

#include "Thread.h"
#include "Socket.h"

class HandshakeThread : public Thread
{
public:
	HandshakeThread(Socket* p);
	~HandshakeThread(void);

private:
	void OnStart() override;

private:
	Socket*		m_pSocket;
};

