#pragma once

#ifndef SEND_H
#define SEND_H

#include "General.h"
#include "Thread.h"

class Send
{
private:
	//THREADS
	//handle of the Send thread
	Thread m_thread;
	//handle of the connecting thread:
	Thread m_connThread;

public:
	//FILE AND DATA
	//the file that is read from and transferred to the other computer:
	static CSourceFile File;
	//how much has been transferred from the global data
	static DWORD dwCurrentPartGlobal;
	//the type of the item sent
	//specifies whether in this transfer will be sent only missing files in the destination directory (if true)
	//or it will write all files - overwriting (if false).
	static BOOL bModeRepair;
	//the number of... great parts?
	static DWORD dwNrGreatParts;
	//the total size: if only one file, the size of that file; if more files, the sum of all sizes.
	static WCHAR wsTotalSize[20];

	//CONNECTION
	//the send socket
	static Socket* pSocket;

	//the type of the item sent
	static ItemType itemType;

	//FILE AND FOLDER NAMES
	//the name of the file/folder that is being transferred
	//if this is the name of the folder, this is only the path for the other items that follow
	static WCHAR* wsParentFileName;
	//the display name of the file/folder that is being transferred
	static WCHAR* wsParentFileDisplayName;
	//if an entire folder is transferred, this is the path of the child item:
	static WCHAR* wsChildFileName;

	//FUNCTIONS
	//sends the Buffer data, with error checking.
	inline BOOL SendData(void* Buffer, int dSize);
	//receives the Buffer data, with error checking.
	inline BOOL ReceiveData(void* Buffer, int dSize);

	//sends the Buffer data, without error checking.
	inline BOOL SendDataSimple(void* Buffer, int dSize);
	//receives the Buffer data, without error checking.
	inline BOOL ReceiveDataSimple(void* Buffer, int dSize);

	//sends the Buffer data, error checking = memcmp
	inline BOOL SendDataShort(void* Buffer, int dSize);
	//receives the Buffer data, error checking = memcmp
	inline BOOL ReceiveDataShort(void* Buffer, int dSize);

	//sends 0 if it is not ready yet to send the file or sends TRUE if a file will follow.
	inline BOOL WaitForDataSend();
	inline BOOL WaitForDataReceive();

	//function for sending one file
	BOOL SendOneFile(WCHAR* wsReadFile, LONGLONG& llSize);

	//the thread for sending data
	static DWORD ThreadProc(void*);
	//thread for connecting:
	static DWORD ConnThreadProc(void*);

	void StopThreads();
	void StartConnThread();
};

#endif//SEND_H