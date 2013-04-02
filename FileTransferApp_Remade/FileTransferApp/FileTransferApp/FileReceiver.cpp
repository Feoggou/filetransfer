#include "FileReceiver.h"
#include "Exceptions.h"
#include "Events.h"
#include "DataTransfer.h"

#include "App.h"

//
//FileSender::FileSender(void)
//{
//}
//
//
//FileSender::~FileSender(void)
//{
//}

void FileReceiver::DoReset()
{
	if (m_file.IsOpened()) m_file.Close();
}

void FileReceiver::WaitForIncommingData()
{
	BYTE dIsReady = 0;

	do
	{
		try
		{
			if (bOrderEnd) return;//false
			Sleep(500);

			m_pDataTransfer->RetrieveData(dIsReady);
			m_pDataTransfer->SendData(dIsReady);
		}
		catch(Exceptions::SocketException& e)
		{
			//if (e.GetEvent().WhatHappened() != Events::NetworkEvent::ConnGracefullyClosed)
			theApp.GetNetworkManager().SendEvent(e.GetEvent());
		}
	}
	while (!dIsReady);
	
}