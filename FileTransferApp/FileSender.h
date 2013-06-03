#pragma once

#include "FileTransferer.h"

class FileSender : public FileTransferer
{
public:
	FileSender(DataTransferer& dataTransferer, const std::wstring& fileName, File& file);
	~FileSender(void);

	bool operator()() override;
};

