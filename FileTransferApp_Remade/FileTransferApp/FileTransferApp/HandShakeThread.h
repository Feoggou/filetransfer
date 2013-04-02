#ifndef HANDSHAKETHREAD_H
#define HANDSHAKETHREAD_H

#include "Thread.h"

class HandShakeThread: public Thread
{
//public:
//	HandShakeThread(void);
//	~HandShakeThread(void);

private:
	virtual void Run()
		throw (Exceptions::SocketException);
};

#endif//HANDSHAKETHREAD_H