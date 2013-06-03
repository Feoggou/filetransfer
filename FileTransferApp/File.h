#pragma once

//#include <windows.h>
#include "General.h"

class File
{
protected:
	//the handle of the file
	HANDLE		m_hFile;
	//the size of the file
	LONGLONG	m_Size;
	//the size of the buffer (max 20 MB)
	DWORD		m_dwDataSize;
	//the buffer
	BYTE*		m_DataBlock;

public:
	//the position in the buffer
	BYTE*		m_pCurrentPos;

public:
	File(void)
	{
		m_hFile = INVALID_HANDLE_VALUE;
		m_Size = m_dwDataSize = 0;
		m_DataBlock = NULL;
		m_pCurrentPos = NULL;
	}

	virtual ~File(void)
	{
		if (m_hFile != INVALID_HANDLE_VALUE)
			CloseHandle(m_hFile);

		if (m_DataBlock)
		{
			delete[] m_DataBlock;
			m_DataBlock = NULL;
		}
	}

	//closes this file
	void Close()
	{
		if (m_hFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
		}

		if (m_DataBlock)
		{
			delete[] m_DataBlock;
			m_DataBlock = NULL;
		}

		m_pCurrentPos = NULL;
	}

	//checks to see if this file is currently opened.
	BOOL IsOpened()
	{
		return (m_hFile != INVALID_HANDLE_VALUE);
	}

	//retrieves the size of the file
	LONGLONG GetSize()
	{
		LARGE_INTEGER liSize;
		GetFileSizeEx(m_hFile, &liSize);
		return liSize.QuadPart;
	}

	virtual BOOL WriteBlock(DWORD dwSize) {return false;}
	virtual BOOL ReadBlock(DWORD& dwSize) {return false;}
	virtual BOOL Create(LPCTSTR wsPath, LARGE_INTEGER& liExptectedSize) {return false;}
};