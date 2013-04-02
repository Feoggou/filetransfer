#ifndef DATATRANSFER_H
#define DATATRANSFER_H

#include "General.h"
#include "NetworkObject.h"
#include "Exceptions.h"

class DataTransfer: public NetworkObject
{
	bool		m_bForServer;
	Socket*		m_pSocket;

public:
	DataTransfer(bool bForServer);
	~DataTransfer(void);

	//for larger data
	void RetrieveData(void* data, int size)
		throw (Exceptions::SocketException);
	//for larger data
	void SendData(void* data, int size)
		throw (Exceptions::SocketException);

	void RetrieveData(BYTE& data)
		throw (Exceptions::SocketException);
	void SendData(BYTE data)
		throw (Exceptions::SocketException);

	void RetrieveData(DWORD& data)
		throw (Exceptions::SocketException);
	void SendData(DWORD data)
		throw (Exceptions::SocketException);

	void RetrieveData(int& data)
		throw (Exceptions::SocketException);
	void SendData(int data)
		throw (Exceptions::SocketException);

	void RetrieveData(bool& data)
		throw (Exceptions::SocketException);
	void SendData(bool data)
		throw (Exceptions::SocketException);

	void RetrieveData(LONGLONG& data)
		throw (Exceptions::SocketException);
	void SendData(LONGLONG data)
		throw (Exceptions::SocketException);
};

//IMPLEMENTATION
inline void DataTransfer::RetrieveData(BYTE& data)
{
	RetrieveData(&data, sizeof(BYTE));
}

inline void DataTransfer::SendData(BYTE data)
{
	SendData(&data, sizeof(BYTE));
}

inline void DataTransfer::RetrieveData(DWORD& data)
{
	RetrieveData(&data, sizeof(DWORD));
}

inline void DataTransfer::SendData(DWORD data)
{
	SendData(&data, sizeof(DWORD));
}

inline void DataTransfer::RetrieveData(int& data)
{
	RetrieveData(&data, sizeof(int));
}

inline void DataTransfer::SendData(int data)
{
	SendData(&data, sizeof(int));
}

inline void DataTransfer::RetrieveData(bool& data)
{
	RetrieveData(&data, sizeof(bool));
}

inline void DataTransfer::SendData(bool data)
{
	SendData(&data, sizeof(bool));
}

inline void DataTransfer::RetrieveData(LONGLONG& data)
{
	RetrieveData(&data, sizeof(LONGLONG));
}

inline void DataTransfer::SendData(LONGLONG data)
{
	SendData(&data, sizeof(LONGLONG));
}

#endif//DATATRANSFER_H