#include "FileSender.h"
#include "Tools.h"
#include "String.h"
#include "MainDlg.h"
#include "Application.h"


FileSender::FileSender(DataTransferer& dataTransferer, const std::wstring& fileName, File& file, const LONGLONG& liSize, bool bRepair, TransferProgress& transferProgress)
	: FileTransferer(dataTransferer, fileName, file, transferProgress),
	m_liSize(liSize),
	m_bRepair(bRepair)
{
}


FileSender::~FileSender(void)
{
}

bool FileSender::operator()()
{
	//now, we open the file and send it to save
	if (FALSE == m_file.Open(m_fileName.data(), &m_liSize)) return false;

	DWORD nrParts = 0;
	{
		m_dataTransferer.SendDataShort(&m_liSize, sizeof(LONGLONG));
		nrParts = (DWORD)m_liSize / BLOCKSIZE;
		if (m_liSize % BLOCKSIZE) nrParts++;
	}

	if (m_bRepair)
	{
		//we need confirmation: whether this file is ok or not
		BYTE exists;
		if (false == m_dataTransferer.ReceiveDataShort(&exists, sizeof(BYTE))) return false;
		//if the file is ok, we skip it:
		if (exists) 
		{
			m_file.Close(); 
			m_transferProgress.EndFile(true, nrParts);

			return true;
		}
	}

	//we send all but the last piece:

	m_transferProgress.BeginFile(m_fileName, nrParts);

	//TODO: "unknown" must be set for batch, not for individual transfers
	//StringCopy(wsTimeLeft, L"unknown");

	DWORD dwRead;

	//transfering the file
	for (DWORD i = 1; i < nrParts; i++)
	{
		if (bOrderEnd) return false;

		//reading from the file
		if (false == m_file.ReadBlock(dwRead)) return false;

		//SENDING DATA
		if (false == m_dataTransferer.SendData(m_file.m_pCurrentPos, dwRead)) return false;

		m_transferProgress.IncreaseProgress(1);
		m_transferProgress.UpdateFileTransferring();
	}

	//now, send the last piece. we do not know how large it is, so we have to read its size first.
	if (false == m_file.ReadBlock(dwRead)) return false;

	//NOW WE TRANSFER THE SIZE OF THE LAST PIECE
	if (false == m_dataTransferer.SendDataShort(&dwRead, sizeof(DWORD))) return false;

	//NOW WE TRANSFER THE LAST PIECE
	if (false == m_dataTransferer.SendData(m_file.m_pCurrentPos, dwRead)) return false;

	m_file.Close();

	m_transferProgress.EndFile(false);

	return true;
}