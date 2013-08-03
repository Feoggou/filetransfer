#include "Thread.h"
#include "Debug.h"


Thread::Thread(void)
	: m_hThread(INVALID_HANDLE_VALUE)
{
}


Thread::~Thread(void)
{
	Close();
}

bool Thread::IsRunning() const
{
	return m_hThread != INVALID_HANDLE_VALUE;
}

void Thread::WaitAndClose()
{
	Wait(-1);

#ifdef _DEBUG
	//Recv::hConnThread should now be closed and set to INVALID_HANDLE_VALUE
	_ASSERTE(m_hThread == INVALID_HANDLE_VALUE);

	Close();
#endif
}

void Thread::WaitAsyncAndClose()
{
	while (WAIT_TIMEOUT == WaitForSingleObject(m_hThread, 100))
	{
		SleepEx(50, TRUE);
	}

	Close();
}

//void Thread::Start(DWORD (*threadFunc)(void*), void* param)
//{
//	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)threadFunc, param, 0, 0);
//}
//
//void Thread::Start(DWORD (*threadFunc)())
//{
//	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)threadFunc, 0, 0, 0);
//}

void Thread::Start()
{
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadFunc, this, 0, 0);
}

void Thread::Close()
{
	if (m_hThread != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hThread);
		m_hThread = INVALID_HANDLE_VALUE;
	}
}

void Thread::Wait(int msecs)
{
	WaitForSingleObject(m_hThread, INFINITE);
}

DWORD Thread::GetExitCode()
{
	DWORD dwExitCode = 1;
	GetExitCodeThread(m_hThread, &dwExitCode);

	return dwExitCode;
}