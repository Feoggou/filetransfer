#ifndef FILESENDINFO_H
#define FILESENDINFO_H

#include "FileTransferer.h"

class DataTransfer;

class FileReceiver: public FileTransferer
{
	//the file that is saved on the disk and written to:
	SaveFile		m_file;
	DataTransfer*	m_pDataTransfer;

public:
	FileReceiver(DataTransfer* const pDataTransfer, bool bForServer): m_pDataTransfer(pDataTransfer){}
	//virtual ~FileSender(void);
	void WaitForIncommingData();

private:
	virtual void DoReset();
};

#endif//FILESENDINFO_H