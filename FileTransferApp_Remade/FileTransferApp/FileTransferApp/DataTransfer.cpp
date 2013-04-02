#include "DataTransfer.h"
#include "SocketClient.h"
#include "SocketServer.h"
#include "Exceptions.h"


DataTransfer::DataTransfer(bool bForServer): m_pSocket(0), 
	m_bForServer(bForServer),
	m_bCancelJob(false)
{
	if (m_bForServer)
	{
		m_pSocket = new SocketServer;
	}

	else
	{
		m_pSocket = new SocketClient;
	}
}


DataTransfer::~DataTransfer(void)
{
	delete m_pSocket;
}

void DataTransfer::RetrieveData(void* data, int size)
{
	int dSentRec;

	//receiving the news: whether a file will follow or not
	m_pSocket->Receive(data, size, dSentRec);
	if (dSentRec <= 0)
	{
		Exceptions::SocketException e(__FILE__, __LINE__, GetLastError());

		if (AbortingJob()) return;//false
		if (0 == dSentRec)
		{
			throw e;
		}

		else if (dSentRec == SOCKET_ERROR)
		{
			throw e;
		}
	}
}

void DataTransfer::SendData(void* data, int size)
{
	int dSentRec;

	//sending the acknowledgement: we have read the message.
	//this value is not used by the other computer.
	m_pSocket->Send(data, size, dSentRec);
	if (dSentRec == SOCKET_ERROR)
	{
		if (bOrderEnd) return;
		
		throw Exceptions::SocketException(__FILE__, __LINE__, GetLastError());
	}
}