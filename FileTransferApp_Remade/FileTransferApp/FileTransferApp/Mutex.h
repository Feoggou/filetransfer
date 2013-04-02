#ifndef MUTEX_H
#define MUTEX_H

class Mutex
{
	HANDLE	m_hMutex;
	bool	m_bLocked;

public:
	Mutex(void);
	~Mutex(void);

	void Lock();
	void Unlock();
};

#endif//MUTEX_H