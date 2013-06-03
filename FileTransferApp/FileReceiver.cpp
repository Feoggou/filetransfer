#include "FileReceiver.h"
#include "General.h"
#include "MainDlg.h"
#include "Application.h"
#include "String.h"
#include "Tools.h"
#include "File.h"


FileReceiver::FileReceiver(DataTransferer& dataTransferer, const std::wstring& fileName, File& file)
	:FileTransferer(dataTransferer, fileName, file)
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
	if (Recv::bModeRepair)
	{
		bool bExists = false;
		//we need to check whether this file is ok or not:
		if (PathFileExistsW(Recv::wsChildFileName)) bExists = true;

		if (false == m_dataTransferer.SendDataShort(&bExists, sizeof(bool))) return false;
		//if the file is ok, we skip it:
		if (true == bExists) 
		{
			Recv::dwCurrentPartGlobal += nrParts;
			if (!MainDlg::m_bIsMinimized)
			{
				SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)Recv::wsChildFileName, 3);
				int oldpos = Recv::dwCurrentPartGlobal * 100 / Recv::dwNrGreatParts;
				PostMessage(MainDlg::m_hBarRecv, PBM_SETPOS, oldpos, 0);
			}
			return true;
		}
	}

	if (FALSE == m_file.Create(Recv::wsChildFileName, liSize))
	{
		return false;
	}

	//we get all but the last piece:
	DWORD oldpos = 0;
		
	LARGE_INTEGER last_count, curr_count, freq;
	QueryPerformanceCounter(&last_count);
	QueryPerformanceFrequency(&freq);

	WCHAR wsTimeLeft[20];
	StringCopy(wsTimeLeft, L"unknown");

	WCHAR wsMessage[300];

	DWORD lasti = 0, deltai = 0;
	float deltax = 0, delta = 0;
	float speed = 0;

	WCHAR wsSpeed[15];
	BOOL bGotInside = FALSE;

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

		Recv::dwCurrentPartGlobal++;

		//we must update the UI
		QueryPerformanceCounter(&curr_count);

		if (!MainDlg::m_bIsMinimized)
		{
			//timpul in secude (float) in care s-au transferat BLOCKSIZE bytes.
			delta = ((curr_count.QuadPart - last_count.QuadPart)/(float)freq.QuadPart);
			//timpul, aprox o secunda, folosit pentru actualizarea UI
			deltax += delta;
			if (deltax >= 1)
			{
				bGotInside = true;
				//numarul de partzi transmise in timpul deltax
				deltai = i - lasti;
				//deltax/deltai : timpul aprox in care ar trebui sa se faca transferul unei singure partzi
				//nrGreatParts - dwCurrentPartGlobal : nr de partzi ramase din toate fisierele, inclusiv cel curent
				//(deltax / deltai) * (nrParts - i) : timpul aprox in care ar trebui sa se faca transferul restului de partzi
				FormatTime(wsTimeLeft, (DWORD)(ceil((deltax/(float)deltai) * (Recv::dwNrGreatParts - Recv::dwCurrentPartGlobal + 1))));
				lasti = i;
			
				//oldpos = pozitia in bara de progress, se actualizeaza aprox. o data pe secunda.
				//oldpos = x, din x% (x = 1->100) finished all.
				oldpos = Recv::dwCurrentPartGlobal * 100 / Recv::dwNrGreatParts;
				PostMessage(MainDlg::m_hBarRecv, PBM_SETPOS, oldpos, 0);
				//viteza = nr de partzi care s-au transferat in timpul deltax * catzi bytes are fiecare parte / deltax
				if (deltai) speed = deltai * BLOCKSIZE / deltax;
				else {speed = 0;}

				SpeedFtoString(speed, wsSpeed);
				StringFormat(wsMessage, L"%d%% of %s; Speed: %s; Time Left: %s", oldpos, Recv::wsTotalSize, wsSpeed, wsTimeLeft);
				SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)wsMessage, 2);
				StringFormat(wsMessage, L"%s", Recv::wsChildFileName);
				SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)Recv::wsChildFileName, 3);

				deltax = 0;
			}
		}

		last_count = curr_count;
	}

	//now, receive the last piece. we do not know how large it is, so we have to read its size first.
	//RECEIVING THE SIZE OF THE LAST PIECE
	DWORD len;
	if (false == m_dataTransferer.ReceiveDataShort(&len, sizeof(DWORD))) return false;

	if (len > BLOCKSIZE)
	{
		MessageBox(theApp->GetMainWindow(), L"len > 10240!", L"EROARE!", 0);
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

	if (!MainDlg::m_bIsMinimized)
	{
		//we update the user interface
		oldpos = Recv::dwCurrentPartGlobal * 100 / Recv::dwNrGreatParts;
		if (!bGotInside)
		{
			speed = nrParts * BLOCKSIZE / deltax;
			SpeedFtoString(speed, wsSpeed);
			FormatTime(wsTimeLeft, (DWORD)(ceil((deltax/(float)nrParts) * (Recv::dwNrGreatParts - Recv::dwCurrentPartGlobal + 1))));
		}

		StringFormat(wsMessage, L"%d%% of %s; Speed: %s; Time Left: %s", oldpos, Recv::wsTotalSize, wsSpeed , wsTimeLeft, Recv::wsChildFileName);
		SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)wsMessage, 2);
		StringFormat(wsMessage, L"%s", m_fileName.data());
		SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)Recv::wsChildFileName, 3);
		PostMessage(MainDlg::m_hBarRecv, PBM_SETPOS, oldpos, 0);
	}
	return true;
}