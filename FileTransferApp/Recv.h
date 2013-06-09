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
	//specifies whether in this transfer will be sent only missing files in the destination directory (if true)
	//or it will write all files - overwriting (if false).
	static BOOL bModeRepair;

	//CONNECTION
	
	//the type of the item sent
	static ItemType itemType;

	//FILE AND FOLDER NAMES
	//the name of the file that is written to, if only one file is transferred.
	//otherwise, the name of the parent folder that is created:
	static WCHAR* wsParentDisplayName;
	//if an entire folder is transferred, this is the path of the current child item:
	static WCHAR* wsChildFileName;

	//the thread for receiving data
	static DWORD ThreadProc(void*);
};