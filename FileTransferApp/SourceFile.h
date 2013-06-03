#pragma once

#ifndef SOURCEFILE_H
#define SOURCEFILE_H

//#include <Windows.h>
#include "File.h"

#include "General.h"

class SourceFile :
	public File
{
private:
	BYTE*	m_pNextPos;

public:
	SourceFile(void);
	~SourceFile(void);

	//opens an exiting file for reading
	BOOL Open(LPCTSTR wsPath, LONGLONG* pllSize) override;

	//reads from a file
	BOOL ReadBlock(DWORD& dwSize) override;
};

#endif//SOURCEFILE_H