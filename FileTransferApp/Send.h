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
	static BOOL bModeRepair;

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

	//the thread for sending data
	static DWORD ThreadProc(void*);
};

#endif//SEND_H