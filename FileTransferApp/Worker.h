#pragma once

#include "Thread.h"
#include "Socket.h"
#include "DataTransferer.h"
#include "File.h"
#include "TransferProgress.h"

class Worker
{
public:
	Worker(bool is_receive);
	virtual ~Worker(void) = 0;

	Socket* GetSocket() {return m_pSocket;}

	void StopThreads();
	void StartConnThread();

	void CloseSocket();
	void CloseFile();

private:
	static DWORD ConnThreadProc(void*);

private:
	//THREADS
	Thread m_transferThread;
	Thread m_connectionThread;

protected:
	Socket*				m_pSocket;
	DataTransferer		m_dataTransferer;
	File*				m_pFile;
	TransferProgress	m_transferProgress;
};

