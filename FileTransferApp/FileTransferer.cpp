#include "FileTransferer.h"


FileTransferer::FileTransferer(DataTransferer& dataTransferer, const std::wstring& fileName, File& file, TransferProgress& transferProgress)
	: m_dataTransferer(dataTransferer),
	m_fileName(fileName),
	m_file(file),
	m_transferProgress(transferProgress)
{
}


FileTransferer::~FileTransferer(void)
{
}

void FileTransferer::CleanUp()
{
	m_file.Close();
}