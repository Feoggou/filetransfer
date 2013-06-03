#pragma once

#include "General.h"
#include "Thread.h"
#include "DestFile.h"
#include "Socket.h"
#include "DataTransferer.h"
#include "Worker.h"

class Recv : public Worker
{
public:
	Recv();

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
	
	//the type of the item sent
	static ItemType itemType;

	//FILE AND FOLDER NAMES
	//the name of the file that is written to, if only one file is transferred.
	//otherwise, the name of the parent folder that is created:
	static WCHAR* wsParentDisplayName;
	//if an entire folder is transferred, this is the path of the current child item:
	static WCHAR* wsChildFileName;

	//receives a file that will be saved with filename (full file name) wsSavePath.
	BOOL ReceiveOneFile();

	//the thread for receiving data
	static DWORD ThreadProc(void*);
};