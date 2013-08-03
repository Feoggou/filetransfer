#pragma once

#include "General.h"
#include "DataTransferer.h"
#include "File.h"
#include "TransferProgress.h"

class Thread
{
public:
	Thread(void);
	virtual ~Thread(void) = 0;

	bool IsRunning() const;
	//TODO: split
	void WaitAndClose();
	//TODO: split
	void WaitAsyncAndClose();
	/*void Start(DWORD (*threadFunc)(void*), void* param);
	void Start(DWORD (*threadFunc)());*/
	void Start();
	void Wait(int msecs = INFINITE);
	void Close();

	DWORD GetExitCode();

private:
	static DWORD ThreadFunc(Thread*);
	//currently called by Start(void)
	virtual void OnStart() = 0;

private:
	HANDLE		m_hThread;
};

class ConnectionThread: public Thread
{
public:
	ConnectionThread() {}

private:
	void OnStart() override {}
};

class TransferFilesThread: public Thread
{
public:
	TransferFilesThread(bool is_receive, HWND hProgressBar)
		: m_dataTransferer(),
		m_pSocket(nullptr),
		m_pFile(nullptr),
		m_transferProgress(!is_receive, hProgressBar)
	{}

protected:
	DataTransferer		m_dataTransferer;
	File*				m_pFile;
	TransferProgress	m_transferProgress;
	Socket*				m_pSocket;
};
