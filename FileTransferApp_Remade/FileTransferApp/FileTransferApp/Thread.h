#ifndef THREAD_H
#define THREAD_H

//#pragma intrinsic

#include "General.h"

class Thread
{
	//the handle of the thread
	HANDLE m_hThread;
	DWORD m_nThreadID;

public:
	//the ctor does not start the thread.
	Thread(void): m_hThread(INVALID_HANDLE_VALUE), m_nThreadID(0) {}
	virtual ~Thread(void) {Destroy();}

	//creates/starts the thread by calling the static ThreadProc, which in turn calls the virtual Run() (defined by subclasses)
	DWORD Create();
	//destroys the thread - also used by the dtor, but it can be used anytime to close the thread.
	void Destroy();

private:
	//runs the code defined by the subclasses of Thread.
	virtual void Run() = 0 throw (Exceptions::SocketException);
	//automatically calls the public pure virtual Run()
	static DWORD ThreadProc(Thread* const pThisThread);
};

#endif//THREAD_H