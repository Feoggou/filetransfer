#include "FileSender.h"


FileSender::FileSender(DataTransferer& dataTransferer, const std::wstring& fileName, File& file)
	: FileTransferer(dataTransferer, fileName, file)
{
}


FileSender::~FileSender(void)
{
}

bool FileSender::operator()()
{
	return false;
}