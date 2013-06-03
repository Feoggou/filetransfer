#include "TransferProgress.h"
#include "MainDlg.h"
#include "Tools.h"
#include "String.h"
#include "Application.h"


TransferProgress::TransferProgress(void)
	: m_delta(0.0f),
	m_dwCurrentPartGlobal(0),
	m_deltax(0),
	m_bGotInside(0),
	m_deltai(0),
	m_dwPartCurrentFile(0),
	m_lasti(0),
	m_dwNrGreatParts(0),
	m_oldpos(0),
	m_speed(0),
	m_countParts(0)
{
	m_curr_count.QuadPart = 0;
	m_last_count.QuadPart = 0;
	m_freq.QuadPart = 0;

	m_wsTimeLeft[0] = '\0';
	m_wsSpeed[0] = '\0';
	m_wsMessage[0] = '\0';
	m_wsTotalSize[0] = '\0';
}


TransferProgress::~TransferProgress(void)
{
}

void TransferProgress::BeginFile(const std::wstring& fileName, DWORD countParts)
{
	m_delta = 0.0f;
	m_curr_count.QuadPart = 0;
	m_last_count.QuadPart = 0;
	m_freq.QuadPart = 0;

	QueryPerformanceCounter(&m_last_count);
	QueryPerformanceFrequency(&m_freq);
	m_deltax = 0;
	m_bGotInside = 0;
	m_deltai = 0;
	m_dwPartCurrentFile = 0;
	m_lasti = 0;

	m_wsTimeLeft[0] = '\0';
	m_oldpos = 0;
	m_speed = 0;
	m_wsSpeed[0] = '\0';
	m_wsMessage[0] = '\0';
	m_wsTotalSize[0] = '\0';

	m_fileName = fileName;
	m_countParts = countParts;
}

void TransferProgress::BeginBatch()
{
	m_dwCurrentPartGlobal = 0;
	m_dwNrGreatParts = 0;

	//m_bRepair = false;
}

void TransferProgress::EndBatch(DWORD dwTime)
{
	WCHAR wsTime[20];
	FormatTime(wsTime, dwTime);

	//we update the user interface
	WCHAR wsMessage[300];
	StringFormat(wsMessage, L"100%% of %s; Speed: 0 KB/s; Time Left: Finished!", m_wsTotalSize);
	SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)wsMessage, 0);
	SendMessage(MainDlg::m_hBarSend, PBM_SETPOS, 100, 0);

	if (Send::itemType == ItemType_File)
	{
		StringFormatW(wsMessage, L"The file has been transfered successfully in %s!", wsTime);
		//we post this message, because the UI thread must perform it and this thread must continue
		//sending keep-alive messages
		PostMessage(theApp->GetMainWindow(), WM_SHOWMESSAGEBOX, 0, (LPARAM)wsMessage);
	}
	else
	{
		StringFormatW(wsMessage, L"The folder has been transfered successfully in %s!", wsTime);
		//we post this message, because the UI thread must perform it and this thread must continue
		//sending keep-alive messages
		PostMessage(theApp->GetMainWindow(), WM_SHOWMESSAGEBOX, 0, (LPARAM)wsMessage);
	}
}

void TransferProgress::EndFile(bool is_skipped, DWORD add_parts)
{
	if (is_skipped)
	{
		m_dwCurrentPartGlobal += add_parts;

		if (MainDlg::m_bIsMinimized)
			return;

		SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)m_fileName.data(), 1);
		int oldpos = m_dwCurrentPartGlobal * 100 / m_dwNrGreatParts;
		PostMessage(MainDlg::m_hBarSend, PBM_SETPOS, oldpos, 0);
	}

	else
	{
		m_dwCurrentPartGlobal++;

		if (MainDlg::m_bIsMinimized)
			return;

		m_oldpos = m_dwCurrentPartGlobal * 100 / m_dwNrGreatParts;
		if (!m_bGotInside)
		{
			m_speed = m_countParts * BLOCKSIZE / m_deltax;
			SpeedFtoString(m_speed, m_wsSpeed);
			FormatTime(m_wsTimeLeft, (DWORD)(ceil((m_deltax/(float)m_countParts) * (m_dwNrGreatParts - m_dwCurrentPartGlobal + 1))));
		}
		StringFormat(m_wsMessage, L"%d%% of %s; Speed: %s; Time Left: %s", m_oldpos, m_wsTotalSize, m_wsSpeed , m_wsTimeLeft);
		SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)m_wsMessage, 0);

		SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)m_fileName.data(), 1);
		PostMessage(MainDlg::m_hBarSend, PBM_SETPOS, m_oldpos, 0);
	}
}

void TransferProgress::IncreaseProgress(int parts)
{
	m_dwPartCurrentFile += parts;
	m_dwCurrentPartGlobal += parts;

	//we must update the UI
	QueryPerformanceCounter(&m_curr_count);
}

void TransferProgress::UpdateFileSending()
{
	m_dwCurrentPartGlobal++;

	//we must update the UI
	QueryPerformanceCounter(&m_curr_count);

	if (!MainDlg::m_bIsMinimized)
	{
		//timpul in secude (float) in care s-au transferat BLOCKSIZE bytes.
		m_delta = ((m_curr_count.QuadPart - m_last_count.QuadPart)/(float)m_freq.QuadPart);
		//timpul, aprox o secunda, folosit pentru actualizarea UI
		m_deltax += m_delta;
		if (m_deltax >= 1)
		{
			m_bGotInside = true;
			//numarul de partzi transmise in timpul deltax
			m_deltai = m_dwPartCurrentFile - m_lasti;
			//deltax/deltai : timpul aprox in care ar trebui sa se faca transferul unei singure partzi
			//nrGreatParts - dwCurrentPartGlobal : nr de partzi ramase din toate fisierele, inclusiv cel curent
			//(deltax / deltai) * (nrParts - i) : timpul aprox in care ar trebui sa se faca transferul restului de partzi
			FormatTime(m_wsTimeLeft, (DWORD)(ceil((m_deltax/(float)m_deltai) * (m_dwNrGreatParts - m_dwCurrentPartGlobal + 1))));
			m_lasti = m_dwPartCurrentFile;

			//oldpos = pozitia in bara de progress, se actualizeaza aprox. o data pe secunda.
			//oldpos = x, din x% (x = 1->100) finished all.
			m_oldpos = m_dwCurrentPartGlobal * 100 / m_dwNrGreatParts;
			PostMessage(MainDlg::m_hBarSend, PBM_SETPOS, m_oldpos, 0);
			//viteza = nr de partzi care s-au transferat in timpul deltax * catzi bytes are fiecare parte / deltax
			if (m_deltai) m_speed = m_deltai * BLOCKSIZE / m_deltax;
			else {m_speed = 0;}

			SpeedFtoString(m_speed, m_wsSpeed);
			StringFormat(m_wsMessage, L"%d%% of %s; Speed: %s; Time Left: %s", m_oldpos, m_wsTotalSize, m_wsSpeed, m_wsTimeLeft);
			SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)m_wsMessage, 0);

			SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)m_fileName.data(), 1);

			m_deltax = 0;
		}
	}

	m_last_count = m_curr_count;
}

void TransferProgress::UpdateSendFinished()
{
}

void TransferProgress::SetFileSize(LONGLONG fileSize)
{
	SizeLLtoString(fileSize, m_wsTotalSize);
	m_dwNrGreatParts = (DWORD) fileSize / BLOCKSIZE;
	if (fileSize % BLOCKSIZE) m_dwNrGreatParts++;
}