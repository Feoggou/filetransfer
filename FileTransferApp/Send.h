#pragma once

#ifndef SEND_H
#define SEND_H

#include "General.h"

namespace Send
{
	//THREADS
	//handle of the Send thread
	extern HANDLE hThread;
	//handle of the connecting thread:
	extern HANDLE hConnThread;

	//FILE AND DATA
	//the file that is read from and transferred to the other computer:
	extern CSourceFile File;
	//how much has been transferred from the global data
	extern DWORD dwCurrentPartGlobal;
	//the type of the item sent
	//specifies whether in this transfer will be sent only missing files in the destination directory (if true)
	//or it will write all files - overwriting (if false).
	extern BOOL bModeRepair;
	//the number of... great parts?
	extern DWORD dwNrGreatParts;
	//the total size: if only one file, the size of that file; if more files, the sum of all sizes.
	extern WCHAR wsTotalSize[20];

	//CONNECTION
	//the send socket
	extern CSamSocket* pSocket;

	//the type of the item sent
	extern ItemType itemType;

	//FILE AND FOLDER NAMES
	//the name of the file/folder that is being transferred
	//if this is the name of the folder, this is only the path for the other items that follow
	extern WCHAR* wsParentFileName;
	//the display name of the file/folder that is being transferred
	extern WCHAR* wsParentFileDisplayName;
	//if an entire folder is transferred, this is the path of the child item:
	extern WCHAR* wsChildFileName;

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
	DWORD ThreadProc(void);
	//thread for connecting:
	DWORD ConnThreadProc(void);
};

#endif//SEND_H