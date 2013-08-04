#include "FileReceiver.h"
#include "General.h"
#include "MainDlg.h"
#include "Application.h"
#include "String.h"
#include "Tools.h"
#include "File.h"


FileReceiver::FileReceiver(DataTransferer& dataTransferer, const std::wstring& fileName, File& file, TransferProgress& transferProgress, bool bModeRepair)
	:FileTransferer(dataTransferer, fileName, file, transferProgress),
	m_bModeRepair(bModeRepair)
{
}


FileReceiver::~FileReceiver(void)
{
}

bool FileReceiver::operator()()
{
	DWORD nrParts = 0;
	LARGE_INTEGER liSize;

	//we first receive the size of the file and calculate its parts
	m_dataTransferer.ReceiveDataShort(&liSize.QuadPart, sizeof(LONGLONG));
	nrParts = (DWORD)liSize.QuadPart / BLOCKSIZE;
	if (liSize.QuadPart % BLOCKSIZE) nrParts++;

	//only if we've been notified that this should be a repair:
	if (m_bModeRepair)
	{
		bool bExists = false;
		//we need to check whether this file is ok or not:
		if (PathFileExistsW(m_fileName.data())) bExists = true;

		if (false == m_dataTransferer.SendDataShort(&bExists, sizeof(bool))) return false;
		//if the file is ok, we skip it:
		if (bExists) 
		{
			m_transferProgress.EndFile(true, nrParts);
		}
	}

	if (FALSE == m_file.Create(m_fileName.data(), liSize))
	{
		return false;
	}

	//we get all but the last piece:
	m_transferProgress.BeginFile(m_fileName, nrParts);

	//transfering the file
	for (DWORD i = 1; i < nrParts; i++)
	{
		if (bOrderEnd) return false;

		//RECEIVING DATA
		if (false == m_dataTransferer.ReceiveData(m_file.m_pCurrentPos, BLOCKSIZE)) return false;
		
		//write the data into the file
		if (false == m_file.WriteBlock(BLOCKSIZE))
		{
			return false;
		}

		m_transferProgress.IncreaseProgress(1);
		m_transferProgress.UpdateFileTransferring();
	}

	//now, receive the last piece. we do not know how large it is, so we have to read its size first.
	//RECEIVING THE SIZE OF THE LAST PIECE
	DWORD len;
	if (false == m_dataTransferer.ReceiveDataShort(&len, sizeof(DWORD))) return false;

	if (len > BLOCKSIZE)
	{
		MessageBox(theApp->GetMainWindow(), L"len > 10240!", L"ERROR!", 0);
#ifdef _DEBUG
		DebugBreak();
#endif
		return false;
	}

	//NOW WE RECEIVE THE LAST PIECE
	if (false == m_dataTransferer.ReceiveData(m_file.m_pCurrentPos, len)) return false;
	
	//we write the last piece to the file
	if (false == m_file.WriteBlock(len)) return false;

	m_file.Close();

	m_transferProgress.EndFile(false);
	
	return true;
}