#include "Thread.h"


Thread::Thread(void)
	: m_hThread(INVALID_HANDLE_VALUE)
{
}


Thread::~Thread(void)
{
}

bool Thread::IsRunning() const
{
	return m_hThread != INVALID_HANDLE_VALUE;
}

void Thread::WaitAndStop()
{
	//it closes itself after finishing
	WaitForSingleObject(m_hThread, INFINITE);

#ifdef _DEBUG
	//Recv::hConnThread should now be closed and set to INVALID_HANDLE_VALUE
	_ASSERTE(m_hThread == INVALID_HANDLE_VALUE);

	Close();
#endif
}

void Thread::WaitAsyncAndStop()
{
	while (WAIT_TIMEOUT == WaitForSingleObject(m_hThread, 100))
	{
		SleepEx(50, TRUE);
	}

	Close();
}

void Thread::Start(DWORD (*threadFunc)(void*), void* param)
{
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)threadFunc, param, 0, 0);
}

//void Thread::Start(DWORD (*threadFunc)())
//{
//	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)threadFunc, 0, 0, 0);
//}

void Thread::Close()
{
	CloseHandle(m_hThread);
	m_hThread = INVALID_HANDLE_VALUE;
}