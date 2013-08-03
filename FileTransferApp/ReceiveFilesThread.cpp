#include "ReceiveFilesThread.h"

#include "MainDlg.h"
#include "String.h"
#include "HandshakeThread.h"
#include "Application.h"
#include "Tools.h"
#include "ConfirmReceive.h"
#include "ConfirmRepair.h"
#include "FileReceiver.h"

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

ReceiveFilesThread::ReceiveFilesThread(void)
	: TransferFilesThread(true, MainDlg::m_hBarRecv),
	m_bModeRepair(false),
	m_itemType(ItemType_File)
{
}


ReceiveFilesThread::~ReceiveFilesThread(void)
{
}

void ReceiveFilesThread::OnStart()
{
	while (bOrderEnd == FALSE && Connected != Conn::NotConnected)
	{
		//cleaning anything that was left
		if (m_pFile->IsOpened()) m_pFile->Close();
		m_wsParentDisplayName.clear();
		m_wsChildFileName.clear();

		m_transferProgress.BeginBatch();

		//here we wait until a file will be transferred!
		if (false == m_dataTransferer.WaitForDataReceive()) return /*0*/;

		PostMessage(MainDlg::m_hBarRecv, PBM_SETPOS, 0, 0);

		//RETRIEVING THE ITEM TYPE
		if (false == m_dataTransferer.ReceiveDataSimple(&m_itemType, sizeof(m_itemType))) return /*false*/;

		if (m_itemType == ItemType_Folder)
		{
			//RECEIVE THE MODE: NORMAL/REPAIR
			if (false == m_dataTransferer.ReceiveDataSimple(&m_bModeRepair, sizeof(m_bModeRepair))) return /*0*/;
		}
		
		//RETRIEVING THE FILENAME LENGTH
		DWORD len = 0;
		if (false == m_dataTransferer.ReceiveDataSimple(&len, sizeof(len))) return /*false*/;

		//RETRIEVING THE FILENAME STRING
		WCHAR* wsParentDisplayName = new WCHAR[len];
		if (false == m_dataTransferer.ReceiveDataSimple(wsParentDisplayName, len * 2)) return /*false*/;
		m_wsParentDisplayName = wsParentDisplayName;
		delete[] wsParentDisplayName;

		//RECEIVEING THE Size of the file/files and optionally, the number of items.
		int nCount = 0;
		LARGE_INTEGER liSize;
		if (m_itemType == ItemType_File)
		{
			//receiving only the size
			if (false == m_dataTransferer.ReceiveDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return /*false*/;
		}
		else
		{
			//receiving, first the nCount and then the liSize
			if (false == m_dataTransferer.ReceiveDataSimple(&nCount, sizeof(nCount))) return /*false*/;

			if (false == m_dataTransferer.ReceiveDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return /*false*/;
		}

		//we create the handshake thread: we send and receive handshake while the user decides
		//to save the item(s) and where or to refuse it.
		//dIsReady = 0;
		//Thread handshake_thread;
		HandshakeThread handshake_thread(m_pSocket);
		handshake_thread.Start();
		
		m_transferProgress.SetFileSize(liSize.QuadPart);

		WCHAR* wsMessage;
		WCHAR *wsTitle = NULL;

		if (m_itemType == ItemType_File)
		{
			int szlen = StringLenW(IDS_INVITE_ACCEPT_FILE);
			szlen += m_wsParentDisplayName.length();

			szlen += m_transferProgress.GetTotalSizeStringLength();
			szlen++;

			wsMessage = new WCHAR[szlen];
			StringFormatW(wsMessage, IDS_INVITE_ACCEPT_FILE, m_wsParentDisplayName, m_transferProgress.GetTotalSizeString());
			
			wsTitle = IDS_TITLE_ACCEPT_FILE;
		}
		else
		{
			if (m_bModeRepair)
			{
				int szlen = StringLenW(IDS_INVITE_REPAIR_FOLDER);
				szlen += m_wsParentDisplayName.length();
				szlen += m_transferProgress.GetTotalSizeStringLength();
				szlen += CountDigits(nCount);
				szlen++;

				wsMessage = new WCHAR[szlen];
				StringFormatW(wsMessage, IDS_INVITE_REPAIR_FOLDER, m_wsParentDisplayName, m_transferProgress.GetTotalSizeString(), nCount);

				wsTitle = IDS_TITLE_REPAIR_FOLDER;
			}
			else
			{
				int szlen = StringLenW(IDS_INVITE_ACCEPT_FOLDER);
				szlen += m_wsParentDisplayName.length();
				szlen += m_transferProgress.GetTotalSizeStringLength();
				szlen += CountDigits(nCount);
				szlen++;

				wsMessage = new WCHAR[szlen];
				StringFormatW(wsMessage, IDS_INVITE_ACCEPT_FOLDER, m_wsParentDisplayName, m_transferProgress.GetTotalSizeString(), nCount);

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
			if (false == m_dataTransferer.SendDataSimple(&bConfirmed, sizeof(bool))) return /*false*/;
			continue;
		}
		delete[] wsMessage;

		std::wstring wsSavePath;

		//if the file was accepted for receiving, we first ask where it should be saved.
		CoInitialize(0);
		if (!m_bModeRepair)
		{
			//Vista & Win7 / XP
			bConfirmed = ConfirmReceive()(m_wsParentDisplayName, wsSavePath, liSize);
		}
		else
		{
			//Vista & Win7 / XP
			bConfirmed = ConfirmRepair()(m_wsParentDisplayName, wsSavePath, liSize);
		}

		CoUninitialize();

		//end the handshake thread
		//dIsReady = 1;
		handshake_thread.Wait();
		DWORD exit_code = handshake_thread.GetExitCode();
		handshake_thread.Close();

		if (0 == exit_code) return /*false*/;

		if (false == m_dataTransferer.SendDataSimple(&bConfirmed, sizeof(bool))) return /*false*/;
		//perhaps the user has canceled receiving the item(s) here.
		if (!bConfirmed) continue;

		//ok, we now have a receive operation.
		dwDataTransfer |= DATATRANSF_RECV;

		//the time here
		LARGE_INTEGER liCountFirst, liCountLast, liFreq;
		QueryPerformanceCounter(&liCountFirst);
		QueryPerformanceFrequency(&liFreq);

		//ok, we have the required size. now, we save the file/folder!
		if (m_itemType == ItemType_File)
		{
			SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Preparing to receive the file...", 2);

			int lenChild = wsSavePath.length() + 1;
			m_wsChildFileName = wsSavePath;

			if (false == FileReceiver(m_dataTransferer, m_wsChildFileName, *m_pFile, m_transferProgress)())
			{
				DeleteFile(m_wsChildFileName.data());
				return /*false*/;
			}

			else  {
				m_transferProgress.IncreaseCurrentPartGlobal();
			}
		}
		else
		{
			PostMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Preparing to receive the folder...", 2);
			int nSaveLen = wsSavePath.length();

			//ok, the file path will be relative to the wsSavePath folder
			//we create the folder at the destination sBase + sFileName.buffer
			if (0 == CreateDirectory(wsSavePath.data(), 0))
			{
				if (ERROR_ALREADY_EXISTS != GetLastError())
				{
					int szlen = StringLen(IDS_FOLDER_COULD_NOT_BE_CREATED);
					szlen += nSaveLen;
					szlen++;

					wsMessage = new WCHAR[szlen];
					StringFormat(wsMessage, L"Folder \"%s\" could not be created.", wsSavePath.data());
					MessageBox(0, wsMessage, L"Error!", 0);

					delete[] wsMessage;
					return /*false*/;
				}
			}

			//pos will point to '\\'
			for (int i = 0; i < nCount; i++)
			{
				ItemType type;
				if (false == m_dataTransferer.ReceiveDataShort(&type, sizeof(int))) return /*false*/;
				if (false == m_dataTransferer.ReceiveDataShort(&len, sizeof(WORD))) return /*false*/;

				//szlen = the total length of the destination file/folder
				int szlen = nSaveLen + len + 1;
				//we will save the text in Recv::WsChildFileName
				m_wsChildFileName = wsSavePath;
				m_wsChildFileName[nSaveLen] = '\\';

				WCHAR path[260];
				
				//add to the chosen path (wsSavePath) the relative name of the file
				if (false == m_dataTransferer.ReceiveData(path, len * 2)) return /*false*/;
				if (type == ItemType_File)
				{
					if (false == FileReceiver(m_dataTransferer, m_wsChildFileName, *m_pFile, m_transferProgress)())
					{
						if (PathFileExistsEx(m_wsChildFileName.data()))
							DeleteFile(m_wsChildFileName.data());
						return /*false*/;
					}

					else  {
						m_transferProgress.IncreaseCurrentPartGlobal();
					}
				}
				else
				{
					//we create the folder at the destination sBase + sFileName.buffer
					if (0 == CreateDirectory(m_wsChildFileName.data(), 0))
					{
						if (ERROR_ALREADY_EXISTS != GetLastError())
						{
							int szlen = StringLen(IDS_FOLDER_COULD_NOT_BE_CREATED);
							szlen += m_wsChildFileName.length();
							szlen++;

							wsMessage = new WCHAR[szlen];
							StringFormat(wsMessage, L"Folder \"%s\" could not be created.", m_wsChildFileName);
							MessageBox(0, wsMessage, L"Error!", 0);

							delete[] wsMessage;
							return /*false*/;
						}
					}
				}
			}
		}

		//clean up
		m_wsParentDisplayName.clear();
		if (m_pFile->IsOpened()) m_pFile->Close();

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
		szlen += m_transferProgress.GetTotalSizeStringLength();
		szlen++;

		wsMessage = new WCHAR[szlen];
		StringFormat(wsMessage, L"100%% of %s; Speed: 0 KB/s; Time Left: Finished!", m_transferProgress.GetTotalSizeString());
		SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)wsMessage, 2);
		delete[] wsMessage;
		
		SendMessage(MainDlg::m_hBarRecv, PBM_SETPOS, 100, 0);

		wsMessage = new WCHAR[300];

		if (m_itemType == ItemType_File)
		{
			StringFormatW(wsMessage, L"The file has been transfered succesfully in %s.\nDo you want to open the folder\
 where you saved the file?", wsTime);
			WCHAR* wPos = StringRevCharW((WCHAR*)wsSavePath.data(), '\\');
			if (wPos) *wPos = 0;
		}
		else
		{
			StringFormatW(wsMessage, L"The folder has been transfered succesfully in %s! nDo you want to open the folder?", wsTime);
		}

		//telling the user that the transfer has ended and asking him whether he wants to open the
		//folder (or containing folder).
		len = wsSavePath.length() + 1;
		//wsPath will be deleted by the UI thread.
		WCHAR* wsPath = new WCHAR[len];
		StringCopy(wsPath, wsSavePath.data());
		//we post this message, because the UI thread must perform it and this thread must continue
		//sending keep-alive messages
		PostMessage(theApp->GetMainWindow(), WM_SHOWMESSAGEBOX, (WPARAM)wsPath, (LPARAM)wsMessage);

		//wsMessage is deleted by the UI thread
		//the UI thread will also update the UI.
	}
	return /*0*/;
}