#ifndef RECVTHREAD_H
#define RECVTHREAD_H

#include "Thread.h"
#include "FileReceiver.h"
#include "DataTransfer.h"
#include "Exceptions.h"
#include <string>

//thread for receiving files & folders from the distant computer.
class RecvThread : public Thread
{
public:
	enum ItemType{Unknown, Folder, File};

	RecvThread(bool bIsServer);

private:
	//PRIVATE DATA:
	DataTransfer	m_dataTransfer;
	FileReceiver	m_fileSendInfo;
	ItemType		m_itemType;
	bool			m_bRepairMode;
	std::wstring	m_wsItemName;
	int				m_nNumberOfItems;
	//if only one file, then the size of that file. Otherwise, the size of all the files combined
	LARGE_INTEGER	m_liTotalItemSize;

	//PRIVATE FUNCTIONS:
	virtual void Run()
		throw (Exceptions::SocketException);

	void RetrieveItemTypeAndMode()
		throw Exceptions::SocketException();

	void RetrieveItemName()
		throw Exceptions::SocketException();

	void RetrieveItemsCountAndSize()
		throw Exceptions::SocketException();
};

#endif//RECVTHREAD_H