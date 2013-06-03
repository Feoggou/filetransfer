#pragma once

#include "DataTransferer.h"
#include "File.h"

#include <string>

class FileTransferer
{
public:
	FileTransferer(DataTransferer& dataTransferer, const std::wstring& fileName, File& file);
	virtual ~FileTransferer(void) = 0;
	void CleanUp();

	virtual bool operator()() = 0;

protected:
	DataTransferer&		m_dataTransferer;
	const std::wstring& m_fileName;
	File&				m_file;
};

