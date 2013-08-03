#pragma once

#include "Thread.h"
#include "DataTransferer.h"
#include "File.h"
#include "TransferProgress.h"

#include <string>

class ReceiveFilesThread : public TransferFilesThread
{
public:
	ReceiveFilesThread(void);
	~ReceiveFilesThread(void);

private:
	void OnStart() override;

private:
	std::wstring m_wsParentDisplayName;
	std::wstring m_wsChildFileName;

	bool		m_bModeRepair;
	ItemType	m_itemType;
};

