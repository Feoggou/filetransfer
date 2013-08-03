#pragma once

#include "Thread.h"
#include "Socket.h"
#include "DataTransferer.h"
#include "File.h"
#include "TransferProgress.h"

#include "ReceiveFilesThread.h"
#include "SendFilesThread.h"

class Worker
{
public:
	Worker(bool is_receive, HWND hProgressBar);
	virtual ~Worker(void) = 0;

	Socket* GetSocket() {return m_pSocket;}

	void StopThreads();
	void StartConnThread(bool server);

	void CloseSocket();
	void CloseFile();

private:
	static DWORD ClientConnThreadProc(void*);
	static DWORD ServerConnThreadProc(void*);

private:
	//THREADS
	TransferFilesThread* m_pTransferThread;
	ConnectionThread m_connectionThread;

protected:
	Socket*				m_pSocket;
};

