#pragma once

#ifndef SAVEFILE_H
#define SAVEFILE_H

//#include <Windows.h>
//#include "samfile.h"
#include "General.h"

class SaveFile :
	public File
{
public:
	SaveFile(void) {};
	~SaveFile(void) {};

	//creates a new file to be written to
	BOOL Create(LPCTSTR wsPath, LARGE_INTEGER& liExptectedSize);

	//closes the file, after it has written to it.
	void Close()
	{
		//if it has been written to the buffer, but the buffer has not been written to the disk (flushed)
		//we flush here and then close the file.
		if (m_pCurrentPos > m_DataBlock)
		{
			DWORD dwWritten;
			if (false == WriteFile(m_hFile, m_DataBlock, m_pCurrentPos - m_DataBlock, &dwWritten, 0))
			{
				DisplayError();
			}
		}

		((File*)this)->Close();
	}

	//writes into a file
	BOOL WriteBlock(DWORD dwSize);
};

#endif//SAVEFILE_H