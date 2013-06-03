#include "FileTransferer.h"


FileTransferer::FileTransferer(DataTransferer& dataTransferer, const std::wstring& fileName, File& file)
	: m_dataTransferer(dataTransferer),
	m_fileName(fileName),
	m_file(file)
{
}


FileTransferer::~FileTransferer(void)
{
}

void FileTransferer::CleanUp()
{
	m_file.Close();
}