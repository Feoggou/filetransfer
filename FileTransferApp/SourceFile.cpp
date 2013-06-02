#include "SourceFile.h"
#include "General.h"

SourceFile::SourceFile(void)
{
}


SourceFile::~SourceFile(void)
{
}

BOOL SourceFile::Open(LPCTSTR wsPath, LONGLONG* pllSize)
{
	_ASSERTE(m_hFile == INVALID_HANDLE_VALUE);
	m_hFile = CreateFileW(wsPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN /*| FILE_FLAG_NO_BUFFERING*/, 0);
	if (INVALID_HANDLE_VALUE == m_hFile)
	{
		DisplayError();
		return FALSE;
	}

	if (pllSize)
	{
		m_Size = *pllSize;

		//the buffer's size is maximum 20 MB
		if (m_Size > 0x1400000)
		{
			m_dwDataSize = 0x1400000;
		}
		else m_dwDataSize = (DWORD)m_Size;

		m_DataBlock = new BYTE[m_dwDataSize];
		m_pCurrentPos = m_pNextPos = m_DataBlock;
	}

	return TRUE;
}

BOOL SourceFile::ReadBlock(DWORD& dwSize)
{
	//m_pCurrentPos must be greater than m_DataBlock (that is, to point inside m_DataBlock)
	//if it doesn't, it means that we have finished 'reading' this large block.
	if (m_pNextPos <= m_DataBlock || m_pNextPos >= m_DataBlock + m_dwDataSize)
	{
		DWORD dwRead = 0;
		if (false == ReadFile(m_hFile, m_DataBlock, m_dwDataSize, &dwRead, 0))
		{
			DisplayError();
			return FALSE;
		}

		//reset the current position
		m_pCurrentPos = m_DataBlock;
		//reset the next position
		m_pNextPos = m_DataBlock + BLOCKSIZE;
		//if we read less than the buffer size (e.g. we are at the end of the file), reset the buffer size.
		if (dwRead < m_dwDataSize)
			m_dwDataSize = dwRead;

		dwSize = dwRead < BLOCKSIZE ? dwRead : BLOCKSIZE;
	}

	//else, it means that we are currently in this block: return a pointer to this buffer and its size
	//and increase the position (m_pCurrentPos)
	else
	{
		//increase the current position and the next position
		m_pCurrentPos += BLOCKSIZE;
		m_pNextPos = m_pCurrentPos + BLOCKSIZE;

		//if the next position will be after the end of the DataBlock
		if (m_pNextPos > m_DataBlock + m_dwDataSize)
		{
			dwSize = m_dwDataSize % BLOCKSIZE;
			m_pCurrentPos = m_DataBlock + m_dwDataSize - dwSize;
		}
		
		else
		{
			dwSize = BLOCKSIZE;
		}
	}

	return TRUE;
}