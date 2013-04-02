#include "Thread.h"
#include "General.h"
#include "Exceptions.h"

void Thread::Destroy()
{
	if (m_hThread != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
}

DWORD Thread::Create()
{
	m_hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Thread::ThreadProc, (void*)this, 0, &m_nThreadID);
	ThrowWinIf(!m_hThread);

	return m_nThreadID;
}

DWORD Thread::ThreadProc(Thread* const pThisThread)
{
	pThisThread->Run();
	return 0;
}