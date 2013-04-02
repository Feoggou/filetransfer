#pragma once

#ifndef STRING_H
#define STRING_H

#include "General.h"

#define String StringW

class StringA
{
private:
	char*		m_sBuffer;
	DWORD		m_dwLength;
	DWORD		m_dwRealSize;

public:
	StringA(void);
	~StringA(void);

	char operator[] (DWORD i)
	{
		return *(m_sBuffer + i);
	}

	DWORD GetRealSize()
	{
		return m_dwRealSize;
	}

	DWORD Length()
	{
		return m_dwLength;
	}

	char* GetBuffer()
	{
		return m_sBuffer;
	}
};

class StringW
{
private:
	WCHAR*		m_wsBuffer;
	DWORD		m_dwLength;
	DWORD		m_dwRealSize;

typedef

public:
	StringW(void)
	{
		m_wsBuffer = NULL;
		m_dwLength = m_dwRealSize = NULL;
	}

	StringW(DWORD dwLen)
	{
		m_wsBuffer = new WCHAR[dwLen];
		m_dwLength = 0;
		m_dwRealSize = dwLen;
	}

	StringW(StringW& wString)
	{
		*this = wString;
	}

	StringW(WCHAR* wsString)
	{
		*this = wsString;
	}

	StringW(StringA& aString)
	{
		*this = aString;
	}

	StringW(char* sString)
	{
		*this = sString;
	}

	~StringW(void)
	{
		if (m_wsBuffer)
		{
			delete m_wsBuffer;
		}
	}

	//StringW
	void operator= (StringW& wString)
	{
		Empty();
		m_dwLength = wString.Length();
		m_dwRealSize = wString.GetRealSize();
		m_wsBuffer = new WCHAR[m_dwLength];
		
		wcscpy(m_wsBuffer, wString.GetBuffer());
	}

	//WCHAR*
	void operator= (WCHAR* wsString)
	{
		Empty();
		m_dwLength = wcslen(wsString);
		m_dwRealSize = m_dwLength;
		m_wsBuffer = new WCHAR[m_dwLength];
		
		wcscpy(m_wsBuffer, wsString);
	}

	//StringA
	void operator= (StringA& aString)
	{
		Empty();
		m_dwLength = aString.Length();
		m_dwRealSize = aString.GetRealSize();
		m_wsBuffer = new WCHAR[m_dwLength];

		for (register int i = 0; i < m_dwLength; i++)
		{
			*(m_wsBuffer + i) = aString[i];
		}
		*(m_wsBuffer + m_dwLength) = 0;
	}

	//char*
	void operator= (char* sString)
	{
		Empty();
		m_dwLength = strlen(sString);
		m_dwRealSize = m_dwLength;
		m_wsBuffer = new WCHAR[m_dwLength];
		
		for (register int i = 0; i < m_dwLength; i++)
		{
			*(m_wsBuffer + i) = *(sString + i);
		}
		*(m_wsBuffer + m_dwLength) = 0;
	}

	//StringW - concatenate
	void operator+= (StringW& wString)
	{
		register DWORD dwLen = wString.Length();
		_ASSERTE(m_dwRealSize - m_dwLength > dwLen);

		for (register int i = m_dwLength; i < m_dwRealSize; i++)
		{
			*(m_wsBuffer + i) = wString[i];
		}
		*(m_wsBuffer + m_dwLength) = 0;
	}

	//WCHAR* - concatenate
	void operator+= (WCHAR* wsString)
	{
		register DWORD dwLen = wcslen(wsString);
		_ASSERTE(m_dwRealSize - m_dwLength > dwLen);

		register int i;
		for (i = 0; i < dwLen; i++)
		{
			*(m_wsBuffer + i + m_dwLength) = *(wsString + i);
		}
		*(m_wsBuffer + i) = 0;
	}

	//StringA - concatenate
	void operator+= (StringA aString)
	{
		register DWORD dwLen = aString.Length();
		_ASSERTE(m_dwRealSize - m_dwLength > dwLen);

		register int i;
		for (i = 0; i < dwLen; i++)
		{
			*(m_wsBuffer + i + m_dwLength) = aString[i];
		}
		*(m_wsBuffer + i) = 0;
	}

	//char* - concatenate
	void operator+= (char* sString)
	{
		register DWORD dwLen = strlen(sString);
		_ASSERTE(m_dwRealSize - m_dwLength > dwLen);

		register int i;
		for (i = 0; i < dwLen; i++)
		{
			*(m_wsBuffer + i + m_dwLength) = *(sString + i);
		}
		*(m_wsBuffer + i) = 0;
	}

	//StringW - equality
	BOOL operator== (StringW& wString)
	{
		register DWORD dwLen = wString.Length();
		
		if (m_dwLength == dwLen)
		{
			for (register DWORD i = 0; i < dwLen; i++)
			{
				if (*(m_wsBuffer + i) != wString[i]) return false;
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	//WCHAR* - equality
	BOOL operator== (WCHAR* wsString)
	{
		register DWORD dwLen = wcslen(wsString);
		
		if (m_dwLength == dwLen)
		{
			for (register DWORD i = 0; i < dwLen; i++)
			{
				if (*(m_wsBuffer + i) != *(wsString + i)) return false;
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	//StringA - equality
	BOOL operator== (StringA& aString)
	{
		register DWORD dwLen = aString.Length();
		
		if (m_dwLength == dwLen)
		{
			for (register DWORD i = 0; i < dwLen; i++)
			{
				if (*(m_wsBuffer + i) != aString[i]) return false;
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	//char* - equality
	BOOL operator== (char* sString)
	{
		register DWORD dwLen = strlen(sString);
		
		if (m_dwLength == dwLen)
		{
			for (register DWORD i = 0; i < dwLen; i++)
			{
				if (*(m_wsBuffer + i) != *(sString + i)) return false;
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	WCHAR operator[] (DWORD i)
	{
		return *(m_wsBuffer + i);
	}

	void Empty()
	{
		if (m_dwLength)
		{
			delete[] m_wsBuffer;
			m_wsBuffer = NULL;
			m_dwLength = 0;
		}
	}

	DWORD GetRealSize()
	{
		return m_dwRealSize;
	}

	DWORD Length()
	{
		return m_dwLength;
	}

	WCHAR* GetBuffer()
	{
		return m_wsBuffer;
	}
};

#endif//STRING_H