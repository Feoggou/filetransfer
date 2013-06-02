#pragma once

class Socket;
#include "General.h"

class DataTransferer
{
public:
	DataTransferer(Socket*);
	DataTransferer();
	~DataTransferer(void);

	void SetSocket(Socket*);

	//FUNCTIONS
	//sends the Buffer data, with error checking
	bool SendData(void* Buffer, int dSize);
	//receives the Buffer data, with error checking.
	bool ReceiveData(void* Buffer, int dSize);

	//sends the Buffer data, without error checking
	bool SendDataSimple(void* Buffer, int dSize);
	//receives the Buffer data, without error checking.
	bool ReceiveDataSimple(void* Buffer, int dSize);

	//sends the Buffer data, error checking = memcmp
	bool SendDataShort(void* Buffer, int dSize);
	//receives the Buffer data, error checking = memcmp
	bool ReceiveDataShort(void* Buffer, int dSize);

	//sends 0 if it is not ready yet to send the file or sends TRUE if a file will follow.
	bool WaitForDataReceive();
	bool WaitForDataSend();
	static DWORD HandShake(void* pv_socket);

private:
	Socket*		m_pSocket;
};

