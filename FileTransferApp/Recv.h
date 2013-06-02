#pragma once

#ifndef RECV_H
#define RECV_H

#include "General.h"
#include "Thread.h"
#include "DestFile.h"
#include "Socket.h"

class Recv
{
private:
	//THREADS
	//handle of the receive thread
	Thread m_thread;
	//handle of the connecting thread:
	Thread m_connThread;

public:
	//FILE AND DATA
	//the file that is saved on the disk and written to:
	static DestFile File;
	//how much has been transferred from the global data
	static DWORD dwCurrentPartGlobal;
	//specifies whether in this transfer will be sent only missing files in the destination directory (if true)
	//or it will write all files - overwriting (if false).
	static BOOL bModeRepair;
	//the number of... great parts?
	static DWORD dwNrGreatParts;
	//the total size: if only one file, the size of that file; if more files, the sum of all sizes.
	static WCHAR wsTotalSize[20];

	//CONNECTION
	//the receive socket
	static Socket* pSocket;
	
	//the type of the item sent
	static ItemType itemType;

	//FILE AND FOLDER NAMES
	//the name of the file that is written to, if only one file is transferred.
	//otherwise, the name of the parent folder that is created:
	static WCHAR* wsParentDisplayName;
	//if an entire folder is transferred, this is the path of the current child item:
	static WCHAR* wsChildFileName;

	//FUNCTIONS
	//sends the Buffer data, with error checking
	inline BOOL SendData(void* Buffer, int dSize);
	//receives the Buffer data, with error checking.
	inline BOOL ReceiveData(void* Buffer, int dSize);

	//sends the Buffer data, without error checking
	inline BOOL SendDataSimple(void* Buffer, int dSize);
	//receives the Buffer data, without error checking.
	inline BOOL ReceiveDataSimple(void* Buffer, int dSize);

	//sends the Buffer data, error checking = memcmp
	inline BOOL SendDataShort(void* Buffer, int dSize);
	//receives the Buffer data, error checking = memcmp
	inline BOOL ReceiveDataShort(void* Buffer, int dSize);

	//sends 0 if it is not ready yet to send the file or sends TRUE if a file will follow.
	inline BOOL WaitForDataReceive();
	static inline BOOL HandShake();

	//receives a file that will be saved with filename (full file name) wsSavePath.
	BOOL ReceiveOneFile();

	//the thread for receiving data
	static DWORD ThreadProc(void*);
	//thread for connecting:
	static DWORD ConnThreadProc(void*);

	void StopThreads();
	void StartConnThread();
};

#endif//RECV_H