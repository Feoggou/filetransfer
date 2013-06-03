#pragma once

#include "Thread.h"
#include "Socket.h"
#include "DataTransferer.h"

class Worker
{
public:
	Worker(void);
	virtual ~Worker(void) = 0;

	Socket* GetSocket() {return m_pSocket;}

	void StopThreads();
	void StartConnThread();

	void CloseSocket();

private:
	static DWORD ConnThreadProc(void*);

private:
	//THREADS
	Thread m_transferThread;
	Thread m_connectionThread;

protected:
	Socket*				m_pSocket;
	DataTransferer		m_dataTransferer;
};

