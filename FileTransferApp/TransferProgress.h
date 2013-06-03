#pragma once

#include <string>

#include "Window.h"

class TransferProgress
{
public:
	TransferProgress(void);
	~TransferProgress(void);

	void UpdateFileSending();
	void UpdateSendFinished();

	void BeginFile(const std::wstring& fileName, DWORD countParts);
	//add parts > 1 only if skipped. if !skipped, add_parts is ignored
	//TODO: split in two
	void EndFile(bool is_skipped, DWORD add_parts = 1);

	void SetFileSize(LONGLONG fileSize);

	void BeginBatch();
	void EndBatch(DWORD dwTime);

	void IncreaseProgress(int parts);

private:
	float			m_delta;
	float			m_deltax;

	LARGE_INTEGER	m_curr_count;
	LARGE_INTEGER	m_last_count;
	LARGE_INTEGER	m_freq;

	DWORD			m_deltai;

	bool			m_bGotInside;
	//repair is not a transfer progress thing
	//bool			m_bRepair;

	DWORD			m_dwPartCurrentFile;
	DWORD			m_dwCurrentPartGlobal;

	//now, i = m_dwPartCurrentFile
	DWORD			m_lasti;

	WCHAR			m_wsTimeLeft[20];
	DWORD			m_dwNrGreatParts;
	DWORD			m_oldpos;
	DWORD			m_countParts;

	float			m_speed;
	WCHAR			m_wsSpeed[15];

	WCHAR			m_wsMessage[300];
	WCHAR			m_wsTotalSize[20];

	std::wstring	m_fileName;
};

