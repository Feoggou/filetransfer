#include "Recv.h"
#include "MainDlg.h"
#include "SocketClient.h"
#include "CRC.h"
#include "General.h"
#include "Tools.h"

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

HANDLE Recv::hThread = INVALID_HANDLE_VALUE;
HANDLE Recv::hConnThread = INVALID_HANDLE_VALUE;

CDestFile Recv::File;
DWORD Recv::dwCurrentPartGlobal = 0;
BOOL Recv::bModeRepair = false;
DWORD Recv::dwNrGreatParts = 0;
WCHAR Recv::wsTotalSize[20];

Socket* Recv::pSocket = NULL;

ItemType Recv::itemType;
WCHAR* Recv::wsParentDisplayName = NULL;
WCHAR* Recv::wsChildFileName = NULL;


inline BOOL Recv::SendData(void* Buffer, int dSize)
{
	_ASSERTE(Buffer);
	int dSentRec;
	CRC crc;
	DWORD dwIsOK = 0;

	do
	{
		//calculate the checksum
		crc = CRCCalc((BYTE*)Buffer, dSize);

		//send the checksum
		pSocket->Send(&crc, sizeof(crc), dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

		//send data
		pSocket->Send(Buffer, dSize, dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

		//receive validation
		pSocket->Receive(&dwIsOK, sizeof(dwIsOK), dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

	//if validation failed try again!
	} while (dwIsOK != 1);

	return true;
}

inline BOOL Recv::ReceiveData(void* Buffer, int dSize)
{
	_ASSERTE(Buffer);
	int dSentRec;
	CRC crc1, crc2;
	DWORD dwIsOK = 0;

	do
	{
		//receive the checksum
		pSocket->Receive(&crc1, sizeof(crc1), dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}
	
		//receive data
		pSocket->Receive(Buffer, dSize, dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

		//calculate the checksum
		crc2 = CRCCalc((BYTE*)Buffer, dSize);

		if (crc1 == crc2)
		{
			dwIsOK = 1;
			pSocket->Send(&dwIsOK, sizeof(dwIsOK), dSentRec);
		}
		else
		{
			dwIsOK = 0;
			pSocket->Send(&dwIsOK, sizeof(dwIsOK), dSentRec);
		}

		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

	//if validation failed try again!
	} while (dwIsOK != 1);

	return true;
}

inline BOOL Recv::SendDataSimple(void* Buffer, int dSize)
{
	_ASSERTE(Buffer);
	int dSentRec;

	//send data
try_again:
	pSocket->Send(Buffer, dSize, dSentRec);
	if (dSentRec == -1)
	{
		if (bOrderEnd) return false;
		goto try_again;
	}

	return true;
}

inline BOOL Recv::ReceiveDataSimple(void* Buffer, int dSize)
{
	_ASSERTE(Buffer);
	int dSentRec;
	
try_again:
	//receive data
	pSocket->Receive(Buffer, dSize, dSentRec);
	if (dSentRec <= 0)
	{
		if (bOrderEnd) return false;
		if (0 == dSentRec)
		{
			SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
			PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
			return false;
		}

		else if (dSentRec == -1)
		{
			goto try_again;
		}
	}

	return true;
}

inline BOOL Recv::SendDataShort(void* Buffer, int dSize)
{
	_ASSERTE(Buffer);
	int dSentRec;
	DWORD dwIsOK = 0;

	do
	{
		//send data
		pSocket->Send(Buffer, dSize, dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

		//receive data
		BYTE* buf2 = new BYTE[dSize];
		pSocket->Receive(buf2, dSize, dSentRec);
		if (dSentRec <= 0)
		{
			delete[] buf2;
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

		//comparing data1 with data2
		if (0 == memcmp(Buffer, buf2, dSize))
		{
			dwIsOK = 1;
		}
		else dwIsOK = 0;

		delete[] buf2;

		//sending validation
		pSocket->Send(&dwIsOK, sizeof(dwIsOK), dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

	//if validation failed try again!
	} while (dwIsOK != 1);

	return true;
}

inline BOOL Recv::ReceiveDataShort(void* Buffer, int dSize)
{
	_ASSERTE(Buffer);
	int dSentRec;
	DWORD dwIsOK = 0;

	do
	{
		//receive data
		pSocket->Receive(Buffer, dSize, dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

		//sending data
		pSocket->Send(Buffer, dSize, dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

		//receiving validation
		pSocket->Receive(&dwIsOK, sizeof(dwIsOK), dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

	//if validation failed try again!
	} while (dwIsOK != 1);

	return true;
}

inline BOOL Recv::WaitForDataReceive()
{
	int dSentRec;
	BYTE dIsReady = 0;

	do
	{
		if (bOrderEnd) return false;
		Sleep(500);

		//receiving the news: whether a file will follow or not
		pSocket->Receive(&dIsReady, sizeof(dIsReady), dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

		//sending the acknowledgement: we have read the message.
		//this value is not used by the other computer.
		pSocket->Send(&dIsReady, sizeof(dIsReady), dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

	}while (!dIsReady);

	return true;
}

BYTE dIsReady = 0;
//the Handshake thread
inline BOOL Recv::HandShake()
{
	int dSentRec;
	BYTE aux;

	do
	{
		if (bOrderEnd) return false;
		Sleep(500);

		//sending the news: ready or not ready to send the file
		pSocket->Send(&dIsReady, sizeof(dIsReady), dSentRec);
		if (dSentRec == -1)
		{
			if (bOrderEnd) return false;
			continue;
		}

		//receiving confirmation: this data does not matter what it is
		pSocket->Receive(&aux, sizeof(aux), dSentRec);
		if (dSentRec <= 0)
		{
			if (bOrderEnd) return false;
			if (0 == dSentRec)
			{
				SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(theApp->GetMainWindow(), WM_CLOSECONNECTION, 0, 0);
				return false;
			}

			else if (dSentRec == -1)
			{
				continue;
			}
		}

	}while (!dIsReady);

	return true;
}

DWORD Recv::ThreadProc(void)
{
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
		if (false == WaitForDataReceive()) return 0;

		PostMessage(MainDlg::m_hBarRecv, PBM_SETPOS, 0, 0);

		//RETRIEVING THE ITEM TYPE
		if (false == ReceiveDataSimple(&Recv::itemType, sizeof(Recv::itemType))) return false;

		if (Recv::itemType == ItemType::Folder)
		{
			//RECEIVE THE MODE: NORMAL/REPAIR
			if (false == ReceiveDataSimple(&Recv::bModeRepair, sizeof(Recv::bModeRepair))) return 0;
		}
		
		//RETRIEVING THE FILENAME LENGTH
		DWORD len = 0;
		if (false == ReceiveDataSimple(&len, sizeof(len))) return false;

		//RETRIEVING THE FILENAME STRING
		Recv::wsParentDisplayName = new WCHAR[len];
		if (false == ReceiveDataSimple(Recv::wsParentDisplayName, len * 2)) return false;

		//RECEIVEING THE Size of the file/files and optionally, the number of items.
		int nCount = 0;
		LARGE_INTEGER liSize;
		if (Recv::itemType == ItemType::File)
		{
			//receiving only the size
			if (false == ReceiveDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return false;
		}
		else
		{
			//receiving, first the nCount and then the liSize
			if (false == ReceiveDataSimple(&nCount, sizeof(nCount))) return false;

			if (false == ReceiveDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return false;
		}

		//we create the handshake thread: we send and receive handshake while the user decides
		//to save the item(s) and where or to refuse it.
		dIsReady = 0;
		HANDLE hHandshake = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)HandShake, 0, 0, 0);
		
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

			dIsReady = 1;
			WaitForSingleObject(hHandshake, INFINITE);
			CloseHandle(hHandshake);

			bConfirmed = false;
			if (false == SendDataSimple(&bConfirmed, sizeof(bool))) return false;
			continue;
		}
		delete[] wsMessage;

		WSTR wsSavePath;

		//if the file was accepted for receiving, we first ask where it should be saved.
		CoInitialize(0);
		if (!bModeRepair)
		{
			//Vista & Win7 / XP
			bConfirmed = Recv::GetConfirmed(&wsSavePath.s, liSize);
			
		}
		else
		{
			//Vista & Win7 / XP
			bConfirmed = GetConfirmedRepair(&wsSavePath.s, liSize);
		}

		CoUninitialize();

		//end the handshake thread
		dIsReady = 1;
		WaitForSingleObject(hHandshake, INFINITE);
		DWORD dwExitCode = 1;
		GetExitCodeThread(hHandshake, &dwExitCode);
		CloseHandle(hHandshake);

		if (0 == dwExitCode) return false;

		if (false == SendDataSimple(&bConfirmed, sizeof(bool))) return false;
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

			if (false == ReceiveOneFile())
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
				if (false == ReceiveDataShort(&type, sizeof(int))) return false;
				if (false == ReceiveDataShort(&len, sizeof(WORD))) return false;

				//szlen = the total length of the destination file/folder
				int szlen = nSaveLen + len + 1;
				//we will save the text in Recv::WsChildFileName
				if (Recv::wsChildFileName) delete[] Recv::wsChildFileName;
				Recv::wsChildFileName = new WCHAR[szlen];
				StringCopy(Recv::wsChildFileName, wsSavePath.s);
				*(Recv::wsChildFileName + nSaveLen) = '\\';
				
				//add to the chosen path (wsSavePath) the relative name of the file
				if (false == ReceiveData(Recv::wsChildFileName + nSaveLen + 1, len * 2)) return false;
				if (type == ItemType::File)
				{
					if (false == ReceiveOneFile())
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

DWORD Recv::ConnThreadProc()
{
	bOrderEnd = false;
	int nError;

	Recv::pSocket = new CSocketClient();
	Send::pSocket = new CSocketClient();
	CSocketClient* pRecvClient = (CSocketClient*)Recv::pSocket;
	CSocketClient* pSendClient = (CSocketClient*)Send::pSocket;

	nError = pRecvClient->Create();
	if (nError)
	{
		DisplayError(nError);
		bOrderEnd = 1;
		goto final;
	}

	nError = pSendClient->Create();
	if (nError)
	{
		DisplayError(nError);
		bOrderEnd = 1;
		goto final;
	}

	SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connecting to the server...");

	//Connect
try_again:
	nError = pSendClient->Connect(14147);//89.40.112.172
	if (nError && !bOrderEnd)
	{
		if (nError == WSAECONNREFUSED) {Sleep(200); goto try_again;}
		if (nError != WSAETIMEDOUT)
		{
			DisplayError(nError);
			bOrderEnd = 1;
			goto final;
		}
		else
		{
			SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connection time-out. Trying again...");
				goto try_again;
		}
	}; 

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
	CloseHandle(Recv::hConnThread);
	Recv::hConnThread = INVALID_HANDLE_VALUE;
	Connected = Conn::ConnAsClient;

	if (!bOrderEnd)
	{
		Recv::hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Recv::ThreadProc, 0, 0, 0);
		Send::hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Send::ThreadProc, 0, 0, 0);

		//after the connection is succesful:
		SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connection to the server has been established.");
		PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonBrowse, 1);
	}
	else PostMessage(theApp->GetMainWindow(), WM_CLOSE, 0, 0);

	return 0;
}

#if _WIN32_WINNT == 0x0600
bool Recv::GetConfirmedVista(WCHAR** wsSavePath, LARGE_INTEGER& liSize)
{
	//we retrieve an IFileSaveDialog object
	IFileSaveDialog* pDlg;
	HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, (void**)&pDlg);
	if (FAILED(hr))
	{
		DisplayError(hr);
		return 0;
	}

	//we retrieve the PIDL of the desktop: needed to retrieve the desktop shell item
	ITEMIDLIST* pidlDesktop;
	hr = SHGetKnownFolderIDList(FOLDERID_Desktop, 0, 0, &pidlDesktop);
	if (FAILED(hr))
	{
		pDlg->Release();

		DisplayError(hr);
		return 0;
	}

	//retrieving the desktop shell item
	IShellItem* pShellItem;
	hr = SHCreateItemFromIDList(pidlDesktop, IID_IShellItem, (void**)&pShellItem);
	if (FAILED(hr))
	{
		pDlg->Release();
		CoTaskMemFree(pidlDesktop);

		DisplayError(hr);
		return 0;
	}

	//we don't need the pidl of the desktop anymore
	CoTaskMemFree(pidlDesktop);

	hr = pDlg->SetDefaultFolder(pShellItem);
	hr = pDlg->SetOptions(FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT);
	hr = pDlg->SetFileName(Recv::wsParentDisplayName);

	BOOL bExit = false;

	do
	{
		//show the dialogbox
		hr = pDlg->Show(theApp->GetMainWindow());
		if (FAILED(hr))
		{
			//either canceled or error
			if (pShellItem)
				pShellItem->Release();
			pDlg->Release();

			if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
				DisplayError(hr);
			return 0;
		}

		//we don't need the desktop shell item.
		if (pShellItem)
		{
			pShellItem->Release();
			pShellItem = NULL;
		}

		//we retrieve the shell item of the selected item
		hr = pDlg->GetResult(&pShellItem);
		if (FAILED(hr))
		{
			pDlg->Release();

			DisplayError(hr);
			return 0;
		}

		//we retrieve the full file name of the selected item	
		WCHAR* wsFileName;
		hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &wsFileName);
		if (FAILED(hr))
		{
			pShellItem->Release();
			pDlg->Release();

			DisplayError(hr);
			return 0;
		}

		pShellItem->Release();
		pShellItem = NULL;

		int len = StringLen(wsFileName);
		len++;

		*wsSavePath = new WCHAR[len];
		StringCopy(*wsSavePath, wsFileName);
		CoTaskMemFree(wsFileName);
		//we set the first '\\' to '\0' so that we would check the free space on this disk
		*(*wsSavePath + 2) = 0;

		//retrieving the free disk space: we need it to see if we can save the file(s) here
		ULARGE_INTEGER freespace = {0};
		GetDiskFreeSpaceExW(*wsSavePath, NULL, NULL, &freespace);
		if (freespace.QuadPart < (ULONGLONG)liSize.QuadPart)
		{
			//ok, not enough disk space: warn and try again
			delete[] *wsSavePath;
			*wsSavePath = NULL;

			MessageBox(0, L"There is not enough disk space on the drive to save the file/folder. Try other drive.", L"Disk Space", 0);
			continue;
		}
		else
		{
			//ok, we have enough disk space. the loop ends here.
			pDlg->Release();

			*(*wsSavePath + 2) = '\\';
			bExit = true;
		}


	} while (!bExit);
	return 1;
}

bool Recv::GetConfirmedRepairVista(WCHAR** wsSavePath, LARGE_INTEGER& liSize)
{
	//REPAIR MODE
	//create an IFileOpenDialog object
	IFileOpenDialog* pDlg;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&pDlg);
	if (FAILED(hr))
	{
		DisplayError(hr);
		return 0;
	}

	//retrieve the PIDL of the desktop - needed to get the shell item of the desktop
	ITEMIDLIST* pidlDesktop;
	hr = SHGetKnownFolderIDList(FOLDERID_Desktop, 0, 0, &pidlDesktop);
	if (FAILED(hr))
	{
		pDlg->Release();

		DisplayError(hr);
		return 0;
	}

	//retrieve the shell item of the desktop
	IShellItem* pShellItem;
	hr = SHCreateItemFromIDList(pidlDesktop, IID_IShellItem, (void**)&pShellItem);
	if (FAILED(hr))
	{
		pDlg->Release();
		CoTaskMemFree(pidlDesktop);

		DisplayError(hr);
		return 0;
	}

	//we don't need the pidl of the desktop anymore
	CoTaskMemFree(pidlDesktop);
	pDlg->SetDefaultFolder(pShellItem);
	pDlg->SetOptions(FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PICKFOLDERS); 
	pDlg->SetFileName(Recv::wsParentDisplayName);

	BOOL bExit = false;
	do
	{
		//we display the dialogbox
		hr = pDlg->Show(theApp->GetMainWindow());
		if (FAILED(hr))
		{
			//canceled or error
			if (pShellItem) pShellItem->Release();
			pDlg->Release();

			if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
				DisplayError(hr);
			return 0;
		}
		//we don't need the shell item of the desktop
		if (pShellItem) pShellItem->Release();

		//we retrieve the shell item of the selected item
		hr = pDlg->GetResult(&pShellItem);
		if (FAILED(hr))
		{
			pDlg->Release();

			DisplayError(hr);
			return 0;
		}
				
		//we retrieve the full file name of the selected item
		WCHAR* wsFileName;
		hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &wsFileName);
		if (FAILED(hr))
		{
			pShellItem->Release();
			pDlg->Release();

			DisplayError(hr);
			return 0;
		}

		IShellFolder* pFolder;
		hr = pShellItem->BindToHandler(NULL, BHID_SFObject, IID_IShellFolder, (void**)&pFolder);
		if (FAILED(hr))
		{
			CoTaskMemFree(wsFileName);
			pShellItem->Release();
			pDlg->Release();

			DisplayError(hr);
			return 0;
		}

		pShellItem->Release();
		pShellItem = NULL;

		int len = StringLen(wsFileName);
		len++;

		*wsSavePath = new WCHAR[len];
		StringCopy(*wsSavePath, wsFileName);
		CoTaskMemFree(wsFileName);
		*(*wsSavePath + 2) = 0;

		LARGE_INTEGER liExistingSize = {0};

		//checking the freespace
		ULARGE_INTEGER freespace = {0};
		GetDiskFreeSpaceExW(*wsSavePath, NULL, NULL, &freespace);
		CDoubleList<FILE_ITEM> Items(OnDestroyFileItemCoTask);
		*(*wsSavePath + 2) = '\\';

		SearchFolder(pFolder, Items, liExistingSize);
		pFolder->Release();

		//nu luam in considerare fisierele care exista deja.
		if (liSize.QuadPart >= liExistingSize.QuadPart)
		{
			if (freespace.QuadPart < (ULONGLONG)liSize.QuadPart - liExistingSize.QuadPart)
			{
				delete[] *wsSavePath;
				*wsSavePath = NULL;

				MessageBox(0, L"There is not enough disk space on the drive to save the file/folder. Try other drive.", L"Disk Space", 0);
			}
			else 
			{
				pDlg->Release();
				bExit = true;
			}
		}
		else
		{
			if (freespace.QuadPart < (ULONGLONG)liSize.QuadPart)
			{
				delete[] *wsSavePath;
				*wsSavePath = NULL;

				MessageBox(0, L"There is not enough disk space on the drive to save the file/folder. Try other drive.", L"Disk Space", 0);
			}
			else 
			{
				pDlg->Release();
				bExit = true;
			}
		}

	}while (!bExit);

	return 1;
}

#else

bool Recv::GetConfirmedXP(WCHAR** wsSavePath, LARGE_INTEGER& liSize)
{
	bool bConfirmed = 1;

	OPENFILENAMEW ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = theApp->GetMainWindow();
	ofn.hInstance = Application::GetHInstance();
	ofn.lpstrFile = new WCHAR[2000];
	ofn.nMaxFile = 2000;
	StringCopy(ofn.lpstrFile, Recv::wsParentDisplayName);
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
				
	BOOL bExit = false;
	BOOL bResult;

	//create a GetSaveFileName dialogbox to choose a directory where to save.
	do
	{
		bResult = GetSaveFileNameW(&ofn);
		if (bResult == false)
		{
			DWORD dwError =  CommDlgExtendedError();
			//error or canceled
			if (dwError)
			{
				WCHAR wsError[500];
				LoadStringW(Application::GetHInstance(), dwError, wsError, 500);
				MessageBox(theApp->GetMainWindow(), wsError, 0, MB_ICONERROR);
			}
						
			delete[] ofn.lpstrFile;
			bExit = true;
			bConfirmed = false;
		}

		else
		{
			//we have chosen a directory.
			int len = StringLen(ofn.lpstrFile);
			len++;

			//copy ofn.lpstrFile into wsSavePath
			*wsSavePath = new WCHAR[len];
			StringCopy(*wsSavePath, ofn.lpstrFile);
			*(*wsSavePath + 2) = 0;

			//retrieve the free disk space and check if we have enough disk space to save the file(s)
			ULARGE_INTEGER freespace = {0};
			GetDiskFreeSpaceExW(*wsSavePath, NULL, NULL, &freespace);
			if (freespace.QuadPart < (ULONGLONG)liSize.QuadPart)
			{
				//ok, we don't have enough free space in this disk: warn
				delete[] *wsSavePath;
				*wsSavePath = NULL;

				MessageBox(0, L"There is not enough disk space on the drive to save the file/folder. Try other drive.", L"Disk Space", 0);
				continue;
			}
			else
			{
				delete[] ofn.lpstrFile;
				//ok, we have enough free disk space: we save here
				*(*wsSavePath + 2) = '\\';
				bExit = true;
			}
		}

	} while (!bExit);

	return bConfirmed;
}

bool Recv::GetConfirmedRepairXP(WCHAR** wsSavePath, LARGE_INTEGER& liSize)
{
	//REPAIR MODE!!
	bool bRetValue = true;

	BROWSEINFOW bi = {0};
	bi.hwndOwner = theApp->GetMainWindow();
	bi.lpszTitle = L"Chose the folder you wish to repair:";
	bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_NONEWFOLDERBUTTON;
	bi.pszDisplayName = new WCHAR[MAX_PATH];
			
	ITEMIDLIST* pidlResult;
	BOOL bExit = false;

	do
	{
		pidlResult = SHBrowseForFolderW(&bi);
		if (pidlResult != 0)
		{
			IShellItem* pShellItem;
			HRESULT hr = SHCreateShellItem(0, 0, pidlResult, &pShellItem);
			if (FAILED(hr))
			{
				CoTaskMemFree(pidlResult);

				DisplayError(hr);
				bRetValue = false;
				break;
			}

			WCHAR* wsFullName;
			hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &wsFullName);
			if (FAILED(hr))
			{
				CoTaskMemFree(pidlResult);
				pShellItem->Release();

				MessageBox(theApp->GetMainWindow(), L"Invalid selection. Chose a directory or a drive!", L"Invalid selection!", MB_ICONWARNING);
				continue;
			}

			CoTaskMemFree(pidlResult);
			pShellItem->Release();

			int len = StringLen(wsFullName);
			len++;
			
			*wsSavePath = new WCHAR[len];
			StringCopy(*wsSavePath, wsFullName);
			CoTaskMemFree(wsFullName);
			*(*wsSavePath + 2) = 0;

			LARGE_INTEGER liExistingSize = {0};

			//checking the freespace
			ULARGE_INTEGER freespace = {0};
			GetDiskFreeSpaceExW(*wsSavePath, NULL, NULL, &freespace);
			CDoubleList<FILE_ITEM> Items(OnDestroyFileItemCoTask);
			*(*wsSavePath + 2) = '\\';

			IShellFolder* pDesktop;
			hr = SHGetDesktopFolder(&pDesktop);
			if (FAILED(hr))
			{
				delete[] *wsSavePath;
				*wsSavePath = NULL;

				DisplayError(hr);
				bRetValue = false;
				break;
			}

			ITEMIDLIST* pidlFolder;
			hr = pDesktop->ParseDisplayName(theApp->GetMainWindow(), NULL, *wsSavePath, NULL, &pidlFolder, NULL);
			if (FAILED(hr))
			{
				pDesktop->Release();
				delete[] *wsSavePath;
				*wsSavePath = NULL;

				DisplayError(hr);
				bRetValue = false;
				break;
			}

			IShellFolder* pFolder;
			hr = pDesktop->BindToObject(pidlFolder, NULL, IID_IShellFolder, (void**)&pFolder);
			if (FAILED(hr))
			{
				pDesktop->Release();
				CoTaskMemFree(pidlFolder);
				delete[] *wsSavePath;
				*wsSavePath = NULL;

				DisplayError(hr);
				bRetValue = false;
				break;
			}

			pDesktop->Release();
			CoTaskMemFree(pidlFolder);

			SearchFolder(pFolder, Items, liExistingSize);
			pFolder->Release();

			//nu luam in considerare fisierele care exista deja.
			if (liSize.QuadPart >= liExistingSize.QuadPart)
			{
				if (freespace.QuadPart < (ULONGLONG)liSize.QuadPart - liExistingSize.QuadPart)
				{
					delete[] *wsSavePath;
					*wsSavePath = NULL;

					MessageBox(0, L"There is not enough disk space on the drive to save the file/folder. Try other drive.", L"Disk Space", 0);
				}
				else 
				{
					bExit = true;
				}
			}
			else
			{
				if (freespace.QuadPart < (ULONGLONG)liSize.QuadPart)
				{
					delete[] *wsSavePath;
					*wsSavePath = NULL;

					MessageBox(0, L"There is not enough disk space on the drive to save the file/folder. Try other drive.", L"Disk Space", 0);
				}
				else 
				{
					bExit = true;
				}
			}
		}
		else bExit = true;

	}while (!bExit);

	delete[] bi.pszDisplayName;

	return bRetValue;
}

#endif

BOOL Recv::ReceiveOneFile()
{
	DWORD nrParts = 0;
	LARGE_INTEGER liSize;

	//we first receive the size of the file and calculate its parts
	ReceiveDataShort(&liSize.QuadPart, sizeof(LONGLONG));
	nrParts = (DWORD)liSize.QuadPart / BLOCKSIZE;
	if (liSize.QuadPart % BLOCKSIZE) nrParts++;

	//only if we've been notified that this should be a repair:
	if (Recv::bModeRepair)
	{
		bool bExists = false;
		//we need to check whether this file is ok or not:
		if (PathFileExistsW(Recv::wsChildFileName)) bExists = true;

		if (false == SendDataShort(&bExists, sizeof(bool))) return false;
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
		if (false == ReceiveData(Recv::File.m_pCurrentPos, BLOCKSIZE)) return false;
		
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
	if (false == ReceiveDataShort(&len, sizeof(DWORD))) return false;

	if (len > BLOCKSIZE)
	{
		MessageBox(theApp->GetMainWindow(), L"len > 10240!", L"EROARE!", 0);
#ifdef _DEBUG
		DebugBreak();
#endif
		return false;
	}

	//NOW WE RECEIVE THE LAST PIECE
	if (false == ReceiveData(Recv::File.m_pCurrentPos, len)) return false;
	
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