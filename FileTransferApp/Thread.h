#pragma once

#include "General.h"

class Thread
{
public:
	Thread(void);
	~Thread(void);

	bool IsRunning() const;
	//TODO: split
	void WaitAndClose();
	//TODO: split
	void WaitAsyncAndClose();
	void Start(DWORD (*threadFunc)(void*), void* param);
	void Start(DWORD (*threadFunc)());
	void Wait(int msecs = INFINITE);
	void Close();

	DWORD GetExitCode();

private:
	HANDLE		m_hThread;
};

