#pragma once

#ifndef SEND_H
#define SEND_H

#include "General.h"
#include "Thread.h"
#include "SourceFile.h"
#include "Socket.h"
#include "DataTransferer.h"
#include "Worker.h"

class Send : public Worker
{
public:
	Send();

	//FILE AND DATA
	//the file that is read from and transferred to the other computer:
	static SourceFile File;
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
	//function for sending one file
	BOOL SendOneFile(WCHAR* wsReadFile, LONGLONG& llSize);

	//the thread for sending data
	static DWORD ThreadProc(void*);
};

#endif//SEND_H