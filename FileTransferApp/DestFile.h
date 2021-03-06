#pragma once

#ifndef DESTFILE_H
#define DESTFILE_H

//#include <Windows.h>
#include "File.h"
#include "General.h"
#include "Debug.h"

class DestFile :
	public File
{
public:
	DestFile(void) {};
	~DestFile(void) {};

	//creates a new file to be written to
	BOOL Create(LPCTSTR wsPath, LARGE_INTEGER& liExptectedSize) override;

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
	BOOL WriteBlock(DWORD dwSize) override;
};

#endif//DESTFILE_H