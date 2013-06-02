#include "DestFile.h"
#include "General.h"

BOOL DestFile::Create(LPCTSTR wsPath, LARGE_INTEGER& liExpectedSize)
{
	//create the file
	_ASSERTE(m_hFile == INVALID_HANDLE_VALUE);
	m_hFile = CreateFileW(wsPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_WRITE_THROUGH /*| FILE_FLAG_NO_BUFFERING*/, 0);
	if (INVALID_HANDLE_VALUE == m_hFile)
	{
		DisplayError();
		return FALSE;
	}

	//set file size:
	if (false == SetFilePointerEx(m_hFile, liExpectedSize, 0, FILE_BEGIN))
	{
		DisplayError();
		Close();
		return FALSE;
	}

	if (false == SetEndOfFile(m_hFile))
	{
		DisplayError();
		Close();
		return FALSE;
	}

	//reseting the position in file:
	if (INVALID_SET_FILE_POINTER == SetFilePointer(m_hFile, 0, 0, FILE_BEGIN))
	{
		DisplayError();
		Close();
		return FALSE;
	}

	m_Size = liExpectedSize.QuadPart;

	//the buffer's size is maximum 20 MB
	if (m_Size > 0x1400000)
	{
		m_dwDataSize = 0x1400000;
	}
	else m_dwDataSize = (DWORD)m_Size;

	m_DataBlock = new BYTE[m_dwDataSize];
	m_pCurrentPos = m_DataBlock;

	return TRUE;
}

BOOL DestFile::WriteBlock(DWORD dwSize)
{
	//write to buffer (if we did not reach the buffer's end)
	//or write to disk if we have reached the end of the buffer.

	//if we can store into the buffer
	if (m_pCurrentPos - m_DataBlock + dwSize </*=*/ m_dwDataSize)
	{
		m_pCurrentPos += dwSize;
	}

	//else, it means that we need to write it on the disk
	else
	{
		DWORD dwWritten = 0;
		if (false == WriteFile(m_hFile, m_DataBlock, m_dwDataSize, &dwWritten, 0))
		{
			DisplayError();
			return FALSE;
		}

		m_pCurrentPos = m_DataBlock;
	}

	return TRUE;
}