#include "RecvThread.h"
#include "App.h"

//to be changed (later on)
//bool bOrderEnd = false;
//Conn Connected = Conn::NotConnected;

RecvThread::RecvThread(bool bIsServer): m_dataTransfer(bIsServer),
	m_fileSendInfo(&m_dataTransfer, bIsServer),
	m_itemType(ItemType::Unknown),
	m_bRepairMode(false),
	m_nNumberOfItems(0)
{
	m_liTotalItemSize.QuadPart = 0;
}

void RecvThread::RetrieveItemTypeAndMode() 
{
	int itemType;
	m_dataTransfer.RetrieveData(itemType);
	m_itemType = static_cast<ItemType>(itemType);

	if (m_itemType == ItemType::Folder)
	{
		//RECEIVE THE MODE: NORMAL/REPAIR
		m_dataTransfer.RetrieveData(m_bRepairMode);
	}
}

void RecvThread::RetrieveItemName()
{
	DWORD len = 0;
	m_dataTransfer.RetrieveData(len);

	//RETRIEVING THE FILENAME STRING
	WCHAR* wsItemName = new WCHAR[len];
	try
	{
		m_dataTransfer.RetrieveData(wsItemName, len * sizeof (WCHAR));
	}
	catch (...)
	{
		delete[] wsItemName;
		throw;
	}

	m_wsItemName = wsItemName;
	delete[] wsItemName;
}

void RecvThread::RetrieveItemsCountAndSize()
{
	//RECEIVEING THE Size of the file/files and optionally, the number of items.
	if (m_itemType == ItemType::File)
	{
		//receiving only the size
		m_dataTransfer.RetrieveData(m_liTotalItemSize.QuadPart);
	}
	else
	{
		//receiving, first the nCount and then the liSize
		m_dataTransfer.RetrieveData(m_nNumberOfItems);
		m_dataTransfer.RetrieveData(m_liTotalItemSize.QuadPart);
	}
}

void RecvThread::Run()
{
	//first of all, we must initialize all sockets

	//while it has not been ordered to end the program we continue to receive data
	while (bOrderEnd == FALSE && Connected != Conn::NotConnected)
	{
		//cleaning anything that was left
		m_fileSendInfo.Reset();

		//here we wait until a file will be transferred!
		m_fileSendInfo.WaitForIncommingData();
		theApp.GetMainDlg().SendEvent(Events::MainDlgEvent::UIEvent::InitReceiveItems);

		//STARTING THE RETRIEVAL OF THE ITEM(S)
		RetrieveItemTypeAndMode();
		RetrieveItemName();
		RetrieveItemsCountAndSize();

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
		if (MessageBox(hMainWnd, wsMessage, wsTitle, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
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
			SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)L"Preparing to receive the file...", 2);

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
			PostMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)L"Preparing to receive the folder...", 2);
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
		SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)wsMessage, 2);
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
		PostMessage(hMainWnd, WM_SHOWMESSAGEBOX, (WPARAM)wsPath, (LPARAM)wsMessage);

		//wsMessage is deleted by the UI thread
		//the UI thread will also update the UI.
	}
	return 0;
}
