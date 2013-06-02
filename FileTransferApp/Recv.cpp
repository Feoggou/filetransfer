#include "Recv.h"
#include "MainDlg.h"
#include "SocketClient.h"
#include "CRC.h"
#include "General.h"
#include "Tools.h"
#include "String.h"
#include "DoubleList.h"
#include "ConfirmReceive.h"
#include "ConfirmRepair.h"

#include "Application.h"

#include <math.h>

#define IDS_INVITE_ACCEPT_FILE L"Your friend wants to send you a file called \"%s\". Do you want to receive it?\n\n\
File Size: %s."
#define IDS_TITLE_ACCEPT_FILE L"Receive File(s)"

#define IDS_INVITE_REPAIR_FOLDER L"Your friend wants to resend you a folder called \"%s\" so that you would repair \
the folder you have on your disk. Do you want to receive it? If yes, you will receive all the files that are missing\
 from your folder but they will not rewrite any file you already have on disk.\n\nContents Size: %s.\nNumber of items: %d"
#define IDS_INVITE_ACCEPT_FOLDER L"Your friend wants to send you a folder called \"%s\". Do you want to receive it?\
\n\nContents Size: %s.\nNumber of items: %d"

#define IDS_TITLE_ACCEPT_FOLDER L"Receive Folder"
#define IDS_TITLE_REPAIR_FOLDER L"Repair Folder"

#define IDS_FOLDER_COULD_NOT_BE_CREATED L"Folder \"%s\" could not be created."
#define IDS_FINISHED_TRANSFERRING L"100%% of %s; Speed: 0 KB/s; Time Left: Finished!"

DestFile Recv::File;
DWORD Recv::dwCurrentPartGlobal = 0;
BOOL Recv::bModeRepair = false;
DWORD Recv::dwNrGreatParts = 0;
WCHAR Recv::wsTotalSize[20];

ItemType Recv::itemType;
WCHAR* Recv::wsParentDisplayName = NULL;
WCHAR* Recv::wsChildFileName = NULL;

Recv::Recv()
	: m_dataTransferer(),
	m_pSocket(NULL)
{
}

DWORD Recv::ThreadProc(void* p)
{
	Recv* pThis = (Recv*)p;
	//first of all, we must initialize all sockets

	//while it has not been ordered to end the program we continue to receive data
	while (bOrderEnd == FALSE && Connected != Conn::NotConnected)
	{
		//cleaning anything that was left
		if (Recv::File.IsOpened()) Recv::File.Close();
		if (Recv::wsParentDisplayName) {delete[] Recv::wsParentDisplayName; Recv::wsParentDisplayName = 0;}
		if (Recv::wsChildFileName) {delete[] Recv::wsChildFileName; Recv::wsChildFileName = 0;}
		Recv::dwCurrentPartGlobal = 0;
		Recv::dwNrGreatParts = 0;

		//here we wait until a file will be transferred!
		if (false == pThis->m_dataTransferer.WaitForDataReceive()) return 0;

		PostMessage(MainDlg::m_hBarRecv, PBM_SETPOS, 0, 0);

		//RETRIEVING THE ITEM TYPE
		if (false == pThis->m_dataTransferer.ReceiveDataSimple(&Recv::itemType, sizeof(Recv::itemType))) return false;

		if (Recv::itemType == ItemType::Folder)
		{
			//RECEIVE THE MODE: NORMAL/REPAIR
			if (false == pThis->m_dataTransferer.ReceiveDataSimple(&Recv::bModeRepair, sizeof(Recv::bModeRepair))) return 0;
		}
		
		//RETRIEVING THE FILENAME LENGTH
		DWORD len = 0;
		if (false == pThis->m_dataTransferer.ReceiveDataSimple(&len, sizeof(len))) return false;

		//RETRIEVING THE FILENAME STRING
		Recv::wsParentDisplayName = new WCHAR[len];
		if (false == pThis->m_dataTransferer.ReceiveDataSimple(Recv::wsParentDisplayName, len * 2)) return false;

		//RECEIVEING THE Size of the file/files and optionally, the number of items.
		int nCount = 0;
		LARGE_INTEGER liSize;
		if (Recv::itemType == ItemType::File)
		{
			//receiving only the size
			if (false == pThis->m_dataTransferer.ReceiveDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return false;
		}
		else
		{
			//receiving, first the nCount and then the liSize
			if (false == pThis->m_dataTransferer.ReceiveDataSimple(&nCount, sizeof(nCount))) return false;

			if (false == pThis->m_dataTransferer.ReceiveDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return false;
		}

		//we create the handshake thread: we send and receive handshake while the user decides
		//to save the item(s) and where or to refuse it.
		//dIsReady = 0;
		Thread handshake_thread;
		handshake_thread.Start(DataTransferer::HandShake, (void*)pThis->m_pSocket);
		
		//format the size to display to the user.
		SizeLItoString(liSize, Recv::wsTotalSize);
		Recv::dwNrGreatParts = (DWORD) liSize.QuadPart / BLOCKSIZE;
		if (liSize.QuadPart % BLOCKSIZE) Recv::dwNrGreatParts++;

		WCHAR* wsMessage;
		WCHAR *wsTitle = NULL;

		if (Recv::itemType == ItemType::File)
		{
			int szlen = StringLenW(IDS_INVITE_ACCEPT_FILE);
			szlen += StringLenW(Recv::wsParentDisplayName);
			szlen += StringLenW(Recv::wsTotalSize);
			szlen++;

			wsMessage = new WCHAR[szlen];
			StringFormatW(wsMessage, IDS_INVITE_ACCEPT_FILE, Recv::wsParentDisplayName, Recv::wsTotalSize);
			
			wsTitle = IDS_TITLE_ACCEPT_FILE;
		}
		else
		{
			if (bModeRepair)
			{
				int szlen = StringLenW(IDS_INVITE_REPAIR_FOLDER);
				szlen += StringLenW(Recv::wsParentDisplayName);
				szlen += StringLenW(Recv::wsTotalSize);
				szlen += CountDigits(nCount);
				szlen++;

				wsMessage = new WCHAR[szlen];
				StringFormatW(wsMessage, IDS_INVITE_REPAIR_FOLDER, Recv::wsParentDisplayName, Recv::wsTotalSize, nCount);

				wsTitle = IDS_TITLE_REPAIR_FOLDER;
			}
			else
			{
				int szlen = StringLenW(IDS_INVITE_ACCEPT_FOLDER);
				szlen += StringLenW(Recv::wsParentDisplayName);
				szlen += StringLenW(Recv::wsTotalSize);
				szlen += CountDigits(nCount);
				szlen++;

				wsMessage = new WCHAR[szlen];
				StringFormatW(wsMessage, IDS_INVITE_ACCEPT_FOLDER, Recv::wsParentDisplayName, wsTotalSize, nCount);

				wsTitle = IDS_TITLE_ACCEPT_FOLDER;
			}
		}

		//asking the user if he wants to receive the file/folder
		bool bConfirmed = true;
		if (MessageBox(theApp->GetMainWindow(), wsMessage, wsTitle, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
		{
			delete[] wsMessage;

			//dIsReady = 1;
			handshake_thread.WaitAndClose();

			bConfirmed = false;
			if (false == pThis->m_dataTransferer.SendDataSimple(&bConfirmed, sizeof(bool))) return false;
			continue;
		}
		delete[] wsMessage;

		WSTR wsSavePath;

		//if the file was accepted for receiving, we first ask where it should be saved.
		CoInitialize(0);
		if (!bModeRepair)
		{
			//Vista & Win7 / XP
			bConfirmed = ConfirmReceive()(wsParentDisplayName, std::wstring(wsSavePath.s), liSize);
		}
		else
		{
			//Vista & Win7 / XP
			bConfirmed = ConfirmRepair()(wsParentDisplayName, std::wstring(wsSavePath.s), liSize);
		}

		CoUninitialize();

		//end the handshake thread
		//dIsReady = 1;
		handshake_thread.Wait();
		DWORD exit_code = handshake_thread.GetExitCode();
		handshake_thread.Close();

		if (0 == exit_code) return false;

		if (false == pThis->m_dataTransferer.SendDataSimple(&bConfirmed, sizeof(bool))) return false;
		//perhaps the user has canceled receiving the item(s) here.
		if (!bConfirmed) continue;

		//ok, we now have a receive operation.
		dwDataTransfer |= DATATRANSF_RECV;

		//the time here
		LARGE_INTEGER liCountFirst, liCountLast, liFreq;
		QueryPerformanceCounter(&liCountFirst);
		QueryPerformanceFrequency(&liFreq);

		//ok, we have the required size. now, we save the file/folder!
		if (Recv::itemType == ItemType::File)
		{
			SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Preparing to receive the file...", 2);

			int lenChild = StringLen(wsSavePath.s) + 1;
			Recv::wsChildFileName = new WCHAR[lenChild];
			StringCopy(Recv::wsChildFileName, wsSavePath.s);

			if (false == pThis->ReceiveOneFile())
			{
				DeleteFile(Recv::wsChildFileName);
				return false;
			}
		}
		else
		{
			PostMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Preparing to receive the folder...", 2);
			int nSaveLen = StringLen(wsSavePath.s);

			//ok, the file path will be relative to the wsSavePath folder
			//we create the folder at the destination sBase + sFileName.buffer
			if (0 == CreateDirectory(wsSavePath.s, 0))
			{
				if (ERROR_ALREADY_EXISTS != GetLastError())
				{
					int szlen = StringLen(IDS_FOLDER_COULD_NOT_BE_CREATED);
					szlen += nSaveLen;
					szlen++;

					wsMessage = new WCHAR[szlen];
					StringFormat(wsMessage, L"Folder \"%s\" could not be created.", wsSavePath.s);
					MessageBox(0, wsMessage, L"Error!", 0);

					delete[] wsMessage;
					return false;
				}
			}

			//pos will point to '\\'
			for (int i = 0; i < nCount; i++)
			{
				ItemType type;
				if (false == pThis->m_dataTransferer.ReceiveDataShort(&type, sizeof(int))) return false;
				if (false == pThis->m_dataTransferer.ReceiveDataShort(&len, sizeof(WORD))) return false;

				//szlen = the total length of the destination file/folder
				int szlen = nSaveLen + len + 1;
				//we will save the text in Recv::WsChildFileName
				if (Recv::wsChildFileName) delete[] Recv::wsChildFileName;
				Recv::wsChildFileName = new WCHAR[szlen];
				StringCopy(Recv::wsChildFileName, wsSavePath.s);
				*(Recv::wsChildFileName + nSaveLen) = '\\';
				
				//add to the chosen path (wsSavePath) the relative name of the file
				if (false == pThis->m_dataTransferer.ReceiveData(Recv::wsChildFileName + nSaveLen + 1, len * 2)) return false;
				if (type == ItemType::File)
				{
					if (false == pThis->ReceiveOneFile())
					{
						if (PathFileExistsEx(Recv::wsChildFileName))
							DeleteFile(Recv::wsChildFileName);
						return false;
					}
				}
				else
				{
					//we create the folder at the destination sBase + sFileName.buffer
					if (0 == CreateDirectory(Recv::wsChildFileName, 0))
					{
						if (ERROR_ALREADY_EXISTS != GetLastError())
						{
							int szlen = StringLen(IDS_FOLDER_COULD_NOT_BE_CREATED);
							szlen += StringLen(Recv::wsChildFileName);
							szlen++;

							wsMessage = new WCHAR[szlen];
							StringFormat(wsMessage, L"Folder \"%s\" could not be created.", Recv::wsChildFileName);
							MessageBox(0, wsMessage, L"Error!", 0);

							delete[] wsMessage;
							return false;
						}
					}
				}
			}
		}

		//clean up
		if (Recv::wsParentDisplayName) {delete[] Recv::wsParentDisplayName; Recv::wsParentDisplayName = 0;}
		if (Recv::File.IsOpened()) Recv::File.Close();

		//update the transfer flag
		dwDataTransfer &= !DATATRANSF_RECV;

		//finalize timer
		QueryPerformanceCounter(&liCountLast);
		double dbTime = (liCountLast.QuadPart - liCountFirst.QuadPart)/(double)liFreq.QuadPart;
		if (dbTime > ceil(dbTime))
		{
			dbTime = ceil(dbTime) + 1;
		}
		else dbTime = ceil(dbTime);

		WCHAR wsTime[20];
		FormatTime(wsTime, (DWORD)dbTime);

		//we update the user interface
		int szlen = StringLen(IDS_FINISHED_TRANSFERRING);
		szlen += StringLen(wsTotalSize);
		szlen++;

		wsMessage = new WCHAR[szlen];
		StringFormat(wsMessage, L"100%% of %s; Speed: 0 KB/s; Time Left: Finished!", wsTotalSize);
		SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)wsMessage, 2);
		delete[] wsMessage;
		
		SendMessage(MainDlg::m_hBarRecv, PBM_SETPOS, 100, 0);

		wsMessage = new WCHAR[300];

		if (itemType == ItemType::File)
		{
			StringFormatW(wsMessage, L"The file has been transfered succesfully in %s.\nDo you want to open the folder\
 where you saved the file?", wsTime);
			WCHAR* wPos = StringRevCharW(wsSavePath.s, '\\');
			if (wPos) *wPos = 0;
		}
		else
		{
			StringFormatW(wsMessage, L"The folder has been transfered succesfully in %s! nDo you want to open the folder?", wsTime);
		}

		//telling the user that the transfer has ended and asking him whether he wants to open the
		//folder (or containing folder).
		len = StringLen(wsSavePath.s) + 1;
		//wsPath will be deleted by the UI thread.
		WCHAR* wsPath = new WCHAR[len];
		StringCopy(wsPath, wsSavePath.s);
		//we post this message, because the UI thread must perform it and this thread must continue
		//sending keep-alive messages
		PostMessage(theApp->GetMainWindow(), WM_SHOWMESSAGEBOX, (WPARAM)wsPath, (LPARAM)wsMessage);

		//wsMessage is deleted by the UI thread
		//the UI thread will also update the UI.
	}
	return 0;
}

#include "Send.h"

DWORD Recv::ConnThreadProc(void* p)
{
	Recv* pThis = (Recv*)p;

	bOrderEnd = false;
	int nError;

	pThis->m_pSocket = new SocketClient();
	//Send::pSocket = new SocketClient();
	SocketClient* pRecvClient = (SocketClient*)pThis->m_pSocket;
	//SocketClient* pSendClient = (SocketClient*)Send::pSocket;

	pThis->m_dataTransferer.SetSocket(pThis->m_pSocket);

	nError = pRecvClient->Create();
	if (nError)
	{
		DisplayError(nError);
		bOrderEnd = 1;
		goto final;
	}

	/*nError = pSendClient->Create();
	if (nError)
	{
		DisplayError(nError);
		bOrderEnd = 1;
		goto final;
	}*/

	SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connecting to the server...");

	//Connect
try_again:
	//nError = pSendClient->Connect(14147);//89.40.112.172
	//if (nError && !bOrderEnd)
	//{
	//	if (nError == WSAECONNREFUSED) {Sleep(200); goto try_again;}
	//	if (nError != WSAETIMEDOUT)
	//	{
	//		DisplayError(nError);
	//		bOrderEnd = 1;
	//		goto final;
	//	}
	//	else
	//	{
	//		SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connection time-out. Trying again...");
	//			goto try_again;
	//	}
	//}; 

try_again2:
	nError = pRecvClient->Connect(14148);
	if (nError && !bOrderEnd)
	{
		if (nError == WSAECONNREFUSED) {Sleep(200); goto try_again2;}
		if (nError != WSAETIMEDOUT)
		{
			DisplayError(nError);
			bOrderEnd = 1;
			goto final;
		}
		else
		{
			SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connection time-out. Trying again...");
				goto try_again2;
		}

	};

final:
	pThis->m_connThread.Close();
	Connected = Conn::ConnAsClient;

	if (!bOrderEnd)
	{
		pThis->m_thread.Start(Recv::ThreadProc, pThis);
		//TODO: why was Send::hThread created here?
		//Send::hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Send::ThreadProc, 0, 0, 0);

		//after the connection is succesful:
		SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connection to the server has been established.");
		PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonBrowse, 1);
	}
	else PostMessage(theApp->GetMainWindow(), WM_CLOSE, 0, 0);

	return 0;
}

BOOL Recv::ReceiveOneFile()
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

	if (FALSE == Recv::File.Create(Recv::wsChildFileName, liSize))
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
		if (false == m_dataTransferer.ReceiveData(Recv::File.m_pCurrentPos, BLOCKSIZE)) return false;
		
		//write the data into the file
		if (false == Recv::File.WriteBlock(BLOCKSIZE))
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
	if (false == m_dataTransferer.ReceiveData(Recv::File.m_pCurrentPos, len)) return false;
	
	//we write the last piece to the file
	if (false == File.WriteBlock(len)) return false;

	Recv::File.Close();

	dwCurrentPartGlobal++;

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
		StringFormat(wsMessage, L"%s", wsChildFileName);
		SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)Recv::wsChildFileName, 3);
		PostMessage(MainDlg::m_hBarRecv, PBM_SETPOS, oldpos, 0);
	}
	return true;
}

void Recv::StopThreads()
{
	if (m_connThread.IsRunning())
	{
		m_connThread.WaitAndClose();
	}

	//first the Recv::hThread
	if (m_thread.IsRunning())
	{
		m_thread.WaitAsyncAndClose();
	}
}

void Recv::StartConnThread()
{
	m_connThread.Start(Recv::ConnThreadProc, this);
}

void Recv::CloseSocket()
{
	if (m_pSocket)
	{
		int nError = m_pSocket->Close();
		if (nError)
		{
			DisplayError(nError);

			delete m_pSocket;
			m_pSocket = NULL;
			PostQuitMessage(-1);
		}

		else
		{
			delete m_pSocket;
			m_pSocket = nullptr;
		}
	}
}