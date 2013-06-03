#pragma once

#include "FileTransferer.h"

class FileSender : public FileTransferer
{
public:
	FileSender(DataTransferer& dataTransferer, const std::wstring& fileName, File& file, const	LONGLONG& liSize, bool repair, TransferProgress& transferProgress);
	~FileSender(void);

	bool operator()() override;

private:
	LONGLONG	m_liSize;
	bool		m_bRepair;
};

