#include "General.h"

#include "Mutex.h"
#include "Exceptions.h"


Mutex::Mutex(void):m_hMutex(0), m_bLocked(false)
{
	WN(m_hMutex = CreateMutex(0, 0, 0));
}


Mutex::~Mutex(void)
{
	if (m_hMutex)
	{
		if (m_bLocked)
		{
			Unlock();
		}

		CloseHandle(m_hMutex);
		m_hMutex = 0;
	}
}

void Mutex::Lock()
{
	DWORD dwResult = WaitForSingleObject(m_hMutex, INFINITE);
	ThrowWinIf(dwResult != WAIT_OBJECT_0);

	m_bLocked = true;
}

void Mutex::Unlock()
{
	WN(ReleaseMutex(m_hMutex));

	m_bLocked = false;
}