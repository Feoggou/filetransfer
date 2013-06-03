#pragma once

#include "FileTransferer.h"

class FileReceiver : public FileTransferer
{
public:
	FileReceiver(DataTransferer& dataTransferer, const std::wstring& fileName, File& file);
	~FileReceiver(void);

	bool operator()() override;
};

