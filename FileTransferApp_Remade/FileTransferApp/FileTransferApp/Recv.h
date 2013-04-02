#pragma once

#ifndef RECV_H
#define RECV_H

#include "General.h"

namespace Recv
{
	//THREADS
	//handle of the receive thread
	extern HANDLE hThread;
	//handle of the connecting thread:
	extern HANDLE hConnThread;

	//FILE AND DATA
	//the file that is saved on the disk and written to:
	extern SaveFile File;
	//how much has been transferred from the global data
	extern DWORD dwCurrentPartGlobal;
	//specifies whether in this transfer will be sent only missing files in the destination directory (if true)
	//or it will write all files - overwriting (if false).
	extern BOOL bModeRepair;
	//the number of... great parts?
	extern DWORD dwNrGreatParts;
	//the total size: if only one file, the size of that file; if more files, the sum of all sizes.
	extern WCHAR wsTotalSize[20];

	//CONNECTION
	//the receive socket
	extern Socket* pSocket;
	
	//the type of the item sent
	extern ItemType itemType;

	//FILE AND FOLDER NAMES
	//the name of the file that is written to, if only one file is transferred.
	//otherwise, the name of the parent folder that is created:
	extern WCHAR* wsParentDisplayName;
	//if an entire folder is transferred, this is the path of the current child item:
	extern WCHAR* wsChildFileName;

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
	inline BOOL HandShake();

	//receives a file that will be saved with filename (full file name) wsSavePath.
	BOOL ReceiveOneFile();

#if _WIN32_WINNT == _WIN32_WINNT_VISTA
#define GetConfirmed		GetConfirmedVista
#define GetConfirmedRepair	GetConfirmedRepairVista

	//opens a Vista-compatible dialogbox to chose a destination where to save the file/folder
	bool GetConfirmedVista(WCHAR** wsSavePath, LARGE_INTEGER& liSize);
	//opens a Vista-compatible dialogbox to chose a folder to repair
	bool GetConfirmedRepairVista(WCHAR** wsSavePath, LARGE_INTEGER& liSize);

#else

#define GetConfirmed		GetConfirmedXP
#define GetConfirmedRepair	GetConfirmedRepairXP

	//opens an XP-compatible dialogbox to chose a destination where to save the file/folder
	bool GetConfirmedXP(WCHAR** wsSavePath, LARGE_INTEGER& liSize);
	//opens an XP-compatible dialogbox to chose a folder to repair
	bool GetConfirmedRepairXP(WCHAR** wsSavePath, LARGE_INTEGER& liSize);
#endif

	//the thread for receiving data
	DWORD ThreadProc(void);
	//thread for connecting:
	DWORD ConnThreadProc(void);
};

#endif//RECV_H