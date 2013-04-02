#pragma once

#ifndef SOURCEFILE_H
#define SOURCEFILE_H

//#include <Windows.h>
//#include "samfile.h"

#include "General.h"

class CSourceFile :
	public File
{
private:
	BYTE*	m_pNextPos;

public:
	CSourceFile(void);
	~CSourceFile(void);

	//opens an exiting file for reading
	BOOL Open(LPCTSTR wsPath, LONGLONG* pllSize = NULL);

	//reads from a file
	BOOL ReadBlock(DWORD& dwSize);
};

#endif//SOURCEFILE_H