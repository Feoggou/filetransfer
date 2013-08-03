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

#include "FileReceiver.h"

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
//
//BOOL Recv::bModeRepair = false;
//
//ItemType Recv::itemType;
//WCHAR* Recv::wsParentDisplayName = NULL;
//WCHAR* Recv::wsChildFileName = NULL;
//
//Recv::Recv()
//	: Worker(/*is receive*/ true, MainDlg::m_hBarRecv)
//{
//}
//
//DWORD Recv::ThreadProc(void* p)
//{
//	Recv* pThis = (Recv*)p;
//	//first of all, we must initialize all sockets
//
//	//while it has not been ordered to end the program we continue to receive data
//	while (bOrderEnd == FALSE && Connected != Conn::NotConnected)
//	{
//		//cleaning anything that was left
//		if (pThis->m_pFile->IsOpened()) pThis->m_pFile->Close();
//		if (Recv::wsParentDisplayName) {delete[] Recv::wsParentDisplayName; Recv::wsParentDisplayName = 0;}
//		if (Recv::wsChildFileName) {delete[] Recv::wsChildFileName; Recv::wsChildFileName = 0;}
//
//		pThis->m_transferProgress.BeginBatch();
//
//		//here we wait until a file will be transferred!
//		if (false == pThis->m_dataTransferer.WaitForDataReceive()) return 0;
//
//		PostMessage(MainDlg::m_hBarRecv, PBM_SETPOS, 0, 0);
//
//		//RETRIEVING THE ITEM TYPE
//		if (false == pThis->m_dataTransferer.ReceiveDataSimple(&Recv::itemType, sizeof(Recv::itemType))) return false;
//
//		if (Recv::itemType == ItemType_Folder)
//		{
//			//RECEIVE THE MODE: NORMAL/REPAIR
//			if (false == pThis->m_dataTransferer.ReceiveDataSimple(&Recv::bModeRepair, sizeof(Recv::bModeRepair))) return 0;
//		}
//		
//		//RETRIEVING THE FILENAME LENGTH
//		DWORD len = 0;
//		if (false == pThis->m_dataTransferer.ReceiveDataSimple(&len, sizeof(len))) return false;
//
//		//RETRIEVING THE FILENAME STRING
//		Recv::wsParentDisplayName = new WCHAR[len];
//		if (false == pThis->m_dataTransferer.ReceiveDataSimple(Recv::wsParentDisplayName, len * 2)) return false;
//
//		//RECEIVEING THE Size of the file/files and optionally, the number of items.
//		int nCount = 0;
//		LARGE_INTEGER liSize;
//		if (Recv::itemType == ItemType_File)
//		{
//			//receiving only the size
//			if (false == pThis->m_dataTransferer.ReceiveDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return false;
//		}
//		else
//		{
//			//receiving, first the nCount and then the liSize
//			if (false == pThis->m_dataTransferer.ReceiveDataSimple(&nCount, sizeof(nCount))) return false;
//
//			if (false == pThis->m_dataTransferer.ReceiveDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return false;
//		}
//
//		//we create the handshake thread: we send and receive handshake while the user decides
//		//to save the item(s) and where or to refuse it.
//		//dIsReady = 0;
//		Thread handshake_thread;
//		handshake_thread.Start(DataTransferer::HandShake, (void*)pThis->m_pSocket);
//		
//		pThis->m_transferProgress.SetFileSize(liSize.QuadPart);
//
//		WCHAR* wsMessage;
//		WCHAR *wsTitle = NULL;
//
//		if (Recv::itemType == ItemType_File)
//		{
//			int szlen = StringLenW(IDS_INVITE_ACCEPT_FILE);
//			szlen += StringLenW(Recv::wsParentDisplayName);
//
//			szlen += pThis->m_transferProgress.GetTotalSizeStringLength();
//			szlen++;
//
//			wsMessage = new WCHAR[szlen];
//			StringFormatW(wsMessage, IDS_INVITE_ACCEPT_FILE, Recv::wsParentDisplayName, pThis->m_transferProgress.GetTotalSizeString());
//			
//			wsTitle = IDS_TITLE_ACCEPT_FILE;
//		}
//		else
//		{
//			if (bModeRepair)
//			{
//				int szlen = StringLenW(IDS_INVITE_REPAIR_FOLDER);
//				szlen += StringLenW(Recv::wsParentDisplayName);
//				szlen += pThis->m_transferProgress.GetTotalSizeStringLength();
//				szlen += CountDigits(nCount);
//				szlen++;
//
//				wsMessage = new WCHAR[szlen];
//				StringFormatW(wsMessage, IDS_INVITE_REPAIR_FOLDER, Recv::wsParentDisplayName, pThis->m_transferProgress.GetTotalSizeString(), nCount);
//
//				wsTitle = IDS_TITLE_REPAIR_FOLDER;
//			}
//			else
//			{
//				int szlen = StringLenW(IDS_INVITE_ACCEPT_FOLDER);
//				szlen += StringLenW(Recv::wsParentDisplayName);
//				szlen += pThis->m_transferProgress.GetTotalSizeStringLength();
//				szlen += CountDigits(nCount);
//				szlen++;
//
//				wsMessage = new WCHAR[szlen];
//				StringFormatW(wsMessage, IDS_INVITE_ACCEPT_FOLDER, Recv::wsParentDisplayName, pThis->m_transferProgress.GetTotalSizeString(), nCount);
//
//				wsTitle = IDS_TITLE_ACCEPT_FOLDER;
//			}
//		}
//
//		//asking the user if he wants to receive the file/folder
//		bool bConfirmed = true;
//		if (MessageBox(theApp->GetMainWindow(), wsMessage, wsTitle, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
//		{
//			delete[] wsMessage;
//
//			//dIsReady = 1;
//			handshake_thread.WaitAndClose();
//
//			bConfirmed = false;
//			if (false == pThis->m_dataTransferer.SendDataSimple(&bConfirmed, sizeof(bool))) return false;
//			continue;
//		}
//		delete[] wsMessage;
//
//		std::wstring wsSavePath;
//
//		//if the file was accepted for receiving, we first ask where it should be saved.
//		CoInitialize(0);
//		if (!bModeRepair)
//		{
//			//Vista & Win7 / XP
//			bConfirmed = ConfirmReceive()(wsParentDisplayName, wsSavePath, liSize);
//		}
//		else
//		{
//			//Vista & Win7 / XP
//			bConfirmed = ConfirmRepair()(wsParentDisplayName, wsSavePath, liSize);
//		}
//
//		CoUninitialize();
//
//		//end the handshake thread
//		//dIsReady = 1;
//		handshake_thread.Wait();
//		DWORD exit_code = handshake_thread.GetExitCode();
//		handshake_thread.Close();
//
//		if (0 == exit_code) return false;
//
//		if (false == pThis->m_dataTransferer.SendDataSimple(&bConfirmed, sizeof(bool))) return false;
//		//perhaps the user has canceled receiving the item(s) here.
//		if (!bConfirmed) continue;
//
//		//ok, we now have a receive operation.
//		dwDataTransfer |= DATATRANSF_RECV;
//
//		//the time here
//		LARGE_INTEGER liCountFirst, liCountLast, liFreq;
//		QueryPerformanceCounter(&liCountFirst);
//		QueryPerformanceFrequency(&liFreq);
//
//		//ok, we have the required size. now, we save the file/folder!
//		if (Recv::itemType == ItemType_File)
//		{
//			SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Preparing to receive the file...", 2);
//
//			int lenChild = wsSavePath.length() + 1;
//			Recv::wsChildFileName = new WCHAR[lenChild];
//			StringCopy(Recv::wsChildFileName, wsSavePath.s);
//
//			if (false == FileReceiver(pThis->m_dataTransferer, wsChildFileName, *pThis->m_pFile, pThis->m_transferProgress)())
//			{
//				DeleteFile(Recv::wsChildFileName);
//				return false;
//			}
//
//			else  {
//				pThis->m_transferProgress.IncreaseCurrentPartGlobal();
//			}
//		}
//		else
//		{
//			PostMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Preparing to receive the folder...", 2);
//			int nSaveLen = StringLen(wsSavePath.s);
//
//			//ok, the file path will be relative to the wsSavePath folder
//			//we create the folder at the destination sBase + sFileName.buffer
//			if (0 == CreateDirectory(wsSavePath.s, 0))
//			{
//				if (ERROR_ALREADY_EXISTS != GetLastError())
//				{
//					int szlen = StringLen(IDS_FOLDER_COULD_NOT_BE_CREATED);
//					szlen += nSaveLen;
//					szlen++;
//
//					wsMessage = new WCHAR[szlen];
//					StringFormat(wsMessage, L"Folder \"%s\" could not be created.", wsSavePath.s);
//					MessageBox(0, wsMessage, L"Error!", 0);
//
//					delete[] wsMessage;
//					return false;
//				}
//			}
//
//			//pos will point to '\\'
//			for (int i = 0; i < nCount; i++)
//			{
//				ItemType type;
//				if (false == pThis->m_dataTransferer.ReceiveDataShort(&type, sizeof(int))) return false;
//				if (false == pThis->m_dataTransferer.ReceiveDataShort(&len, sizeof(WORD))) return false;
//
//				//szlen = the total length of the destination file/folder
//				int szlen = nSaveLen + len + 1;
//				//we will save the text in Recv::WsChildFileName
//				if (Recv::wsChildFileName) delete[] Recv::wsChildFileName;
//				Recv::wsChildFileName = new WCHAR[szlen];
//				StringCopy(Recv::wsChildFileName, wsSavePath.s);
//				*(Recv::wsChildFileName + nSaveLen) = '\\';
//				
//				//add to the chosen path (wsSavePath) the relative name of the file
//				if (false == pThis->m_dataTransferer.ReceiveData(Recv::wsChildFileName + nSaveLen + 1, len * 2)) return false;
//				if (type == ItemType_File)
//				{
//					if (false == FileReceiver(pThis->m_dataTransferer, wsChildFileName, *pThis->m_pFile, pThis->m_transferProgress)())
//					{
//						if (PathFileExistsEx(Recv::wsChildFileName))
//							DeleteFile(Recv::wsChildFileName);
//						return false;
//					}
//
//					else  {
//						pThis->m_transferProgress.IncreaseCurrentPartGlobal();
//					}
//				}
//				else
//				{
//					//we create the folder at the destination sBase + sFileName.buffer
//					if (0 == CreateDirectory(Recv::wsChildFileName, 0))
//					{
//						if (ERROR_ALREADY_EXISTS != GetLastError())
//						{
//							int szlen = StringLen(IDS_FOLDER_COULD_NOT_BE_CREATED);
//							szlen += StringLen(Recv::wsChildFileName);
//							szlen++;
//
//							wsMessage = new WCHAR[szlen];
//							StringFormat(wsMessage, L"Folder \"%s\" could not be created.", Recv::wsChildFileName);
//							MessageBox(0, wsMessage, L"Error!", 0);
//
//							delete[] wsMessage;
//							return false;
//						}
//					}
//				}
//			}
//		}
//
//		//clean up
//		if (Recv::wsParentDisplayName) {delete[] Recv::wsParentDisplayName; Recv::wsParentDisplayName = 0;}
//		if (pThis->m_pFile->IsOpened()) pThis->m_pFile->Close();
//
//		//update the transfer flag
//		dwDataTransfer &= !DATATRANSF_RECV;
//
//		//finalize timer
//		QueryPerformanceCounter(&liCountLast);
//		double dbTime = (liCountLast.QuadPart - liCountFirst.QuadPart)/(double)liFreq.QuadPart;
//		if (dbTime > ceil(dbTime))
//		{
//			dbTime = ceil(dbTime) + 1;
//		}
//		else dbTime = ceil(dbTime);
//
//		WCHAR wsTime[20];
//		FormatTime(wsTime, (DWORD)dbTime);
//
//		//we update the user interface
//		int szlen = StringLen(IDS_FINISHED_TRANSFERRING);
//		szlen += pThis->m_transferProgress.GetTotalSizeStringLength();
//		szlen++;
//
//		wsMessage = new WCHAR[szlen];
//		StringFormat(wsMessage, L"100%% of %s; Speed: 0 KB/s; Time Left: Finished!", pThis->m_transferProgress.GetTotalSizeString());
//		SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)wsMessage, 2);
//		delete[] wsMessage;
//		
//		SendMessage(MainDlg::m_hBarRecv, PBM_SETPOS, 100, 0);
//
//		wsMessage = new WCHAR[300];
//
//		if (itemType == ItemType_File)
//		{
//			StringFormatW(wsMessage, L"The file has been transfered succesfully in %s.\nDo you want to open the folder\
// where you saved the file?", wsTime);
//			WCHAR* wPos = StringRevCharW(wsSavePath.s, '\\');
//			if (wPos) *wPos = 0;
//		}
//		else
//		{
//			StringFormatW(wsMessage, L"The folder has been transfered succesfully in %s! nDo you want to open the folder?", wsTime);
//		}
//
//		//telling the user that the transfer has ended and asking him whether he wants to open the
//		//folder (or containing folder).
//		len = StringLen(wsSavePath.s) + 1;
//		//wsPath will be deleted by the UI thread.
//		WCHAR* wsPath = new WCHAR[len];
//		StringCopy(wsPath, wsSavePath.s);
//		//we post this message, because the UI thread must perform it and this thread must continue
//		//sending keep-alive messages
//		PostMessage(theApp->GetMainWindow(), WM_SHOWMESSAGEBOX, (WPARAM)wsPath, (LPARAM)wsMessage);
//
//		//wsMessage is deleted by the UI thread
//		//the UI thread will also update the UI.
//	}
//	return 0;
//}