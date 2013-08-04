#pragma once

#include "FileTransferer.h"

class FileReceiver : public FileTransferer
{
public:
	FileReceiver(DataTransferer& dataTransferer, const std::wstring& fileName, File& file, TransferProgress& transferProgress, bool bModeRepair);
	~FileReceiver(void);

	bool operator()() override;

private:
	bool m_bModeRepair;
};

