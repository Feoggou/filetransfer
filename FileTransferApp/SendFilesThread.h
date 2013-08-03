#pragma once

#include "Thread.h"
#include "DataTransferer.h"
#include "File.h"
#include "TransferProgress.h"

class SendFilesThread : public TransferFilesThread
{
public:
	SendFilesThread(void);
	~SendFilesThread(void);

private:
	void OnStart() override;
};

