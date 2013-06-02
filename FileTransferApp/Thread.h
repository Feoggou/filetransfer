#pragma once

#include "General.h"

class Thread
{
public:
	Thread(void);
	~Thread(void);

	bool IsRunning() const;
	void WaitAndStop();
	void WaitAsyncAndStop();
	void Start(DWORD (*threadFunc)(void*), void* param);
	//void Start(DWORD (*threadFunc)());
	void Close();

private:
	HANDLE		m_hThread;
};

