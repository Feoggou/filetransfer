#ifndef FILETRANSFERINFO_H
#define FILETRANSFERINFO_H

#include "General.h"
#include "NetworkObject.h"

//used for keeping data regarding the current file/folder transfer.
class FileTransferer: public NetworkObject
{
	//to be changed: now we're prototyping!
	wstring			m_wsParentDisplayName;
	wstring			m_wsChildFileName;

	//how much has been transferred from the global data
	DWORD			m_dwCurrentPartGlobal;
	//the number of... great parts?
	DWORD			m_dwNrGreatParts;

public:
	FileTransferer(void): m_dwCurrentPartGlobal(0),
		m_dwNrGreatParts(0){}

	virtual ~FileTransferer(void);

	//resets the data/file transfer information. Calls the private virtual DoReset to do the reset for the subclasses
	void Reset();

private:
	//not implemented on FileTransferer. must be implemented by the children of this class.
	virtual void DoReset(){}
};

#endif//FILETRANSFER_H