#include "FileTransferer.h"


void FileTransferer::Reset()
{
	m_wsParentDisplayName.clear();
	m_wsChildFileName.clear();
	m_dwCurrentPartGlobal = 0;
	m_dwNrGreatParts = 0;

	DoReset();
}