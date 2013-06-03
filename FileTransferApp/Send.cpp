#include "Send.h"
#include "MainDlg.h"
#include "SocketServer.h"
#include "CRC.h"
#include "Tools.h"
#include "String.h"
#include "FileSender.h"

#include "Application.h"

#include <math.h>

SourceFile Send::File;
BOOL Send::bModeRepair = false;
ItemType Send::itemType;

WCHAR* Send::wsParentFileName = NULL;
WCHAR* Send::wsParentFileDisplayName = NULL;
WCHAR* Send::wsChildFileName = NULL;

Send::Send()
	: Worker(/*is receive*/ false)
{
}

DWORD Send::ThreadProc(void* p)
{
	Send* pThis = (Send*)p;

	//while it has not been ordered to end the program we continue to transfer data
	while (bOrderEnd == FALSE && Connected != Conn::NotConnected)
	{
		//if it was not pressed the Send button, we do a handshake
		if (false == pThis->m_dataTransferer.WaitForDataSend()) return 0;

		//cleaning anything that was left:
		if (Send::File.IsOpened()) Send::File.Close();
		//we do not remove Send::wsParentFileName and Send::wsParentDisplayName because they were chosen in BROWSE and we need them NOW

		pThis->m_transferProgress.BeginBatch();
		Send::bModeRepair = 0;

		PostMessage(MainDlg::m_hBarSend, PBM_SETPOS, 0, 0);
		PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hCheckRepairMode, 0);
		
		//SENDING THE ITEM TYPE
		if (false == pThis->m_dataTransferer.SendDataSimple(&Send::itemType, sizeof(Send::itemType))) return 0;

		//ONLY IF itemType == ItemType_Folder
		if (Send::itemType == ItemType_Folder)
		{
			//SENDING THE MODE: NORMAL/REPAIR
			if (BST_CHECKED == SendMessage(MainDlg::m_hCheckRepairMode, BM_GETCHECK, 0, 0))
				Send::bModeRepair = TRUE;
			else Send::bModeRepair = FALSE;

			if (false == pThis->m_dataTransferer.SendDataSimple(&Send::bModeRepair, sizeof(Send::bModeRepair))) return 0;
		}

		//SENDING THE FILENAME LENGTH
		//getting the length from the string
		DWORD len = StringLen(Send::wsParentFileDisplayName);
		len++;

		if (false == pThis->m_dataTransferer.SendDataSimple(&len, sizeof(len))) return 0;
		
		//SENDING THE FILENAME STRING
		if (false == pThis->m_dataTransferer.SendDataSimple(Send::wsParentFileDisplayName, len * 2)) return 0;

		int nCount = 0;
		LARGE_INTEGER liSize = {0};

		//CALCULATING AND SENDING FILE/FILES SIZE
		CDoubleList<FILE_ITEM> Items(OnDestroyFileItemCoTask);
		if (Send::itemType == ItemType_File)
		{
			if (false == CalcFileSize(Send::wsParentFileName, liSize)) 
			{
				MessageBox(0, L"And error has occured while trying to calculate the size of the specified file", 0, 0);
				continue;
			}
			if (false == pThis->m_dataTransferer.SendDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return 0;
		}
		else
		{
			IShellFolder* pDesktop;
			HRESULT hr = SHGetDesktopFolder(&pDesktop);
			if (FAILED(hr))
			{
				DisplayError(hr);
				return false;
			}

			ITEMIDLIST* pidlFolder;
			hr = pDesktop->ParseDisplayName(theApp->GetMainWindow(), NULL, Send::wsParentFileName, NULL, &pidlFolder, NULL);
			if (FAILED(hr))
			{
				pDesktop->Release();
				DisplayError(hr);
				return false;
			}

			IShellFolder* pFolder;
			hr = pDesktop->BindToObject(pidlFolder, NULL, IID_IShellFolder, (void**)&pFolder);
			if (FAILED(hr))
			{
				pDesktop->Release();
				CoTaskMemFree(pidlFolder);

				DisplayError(hr);
				return false;
			}

			pDesktop->Release();
			CoTaskMemFree(pidlFolder);

			SearchFolder(pFolder, Items, liSize);
			pFolder->Release();
			nCount = Items.size();
			
			//WE NOW SEND THE nCount AND liSize DATA
			if (false == pThis->m_dataTransferer.SendDataSimple(&nCount, sizeof(nCount))) return 0;
			if (false == pThis->m_dataTransferer.SendDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return 0;
		}

		pThis->m_transferProgress.SetFileSize(liSize.QuadPart);

		//RECEIVING THE CONFIRMATION: whether the transfer is allowed or denied:
		if (Send::itemType == ItemType_File)
			SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Asking your friend to receive the file...", 0);
		else
			SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Asking your friend to receive the folder...", 0);

		bool bConfirmed = 0;
		//we wait until the other user decides whether to save the item or to refuse it.
		if (false == pThis->m_dataTransferer.WaitForDataReceive()) return 0;
		if (false == pThis->m_dataTransferer.ReceiveDataSimple(&bConfirmed, sizeof(bool))) {return 0;}

		if (bConfirmed == 0) 
		{
			if (Send::itemType == ItemType_File)
				SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"The file has been refused...", 0);
			else
				SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"The folder has been refused...", 0);

			//WE MUST ALLOW BROWSE AND SEND AND REPAIR!!
			PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonSend, 1);
			PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonBrowse, 1);
			PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hCheckRepairMode, 1);
			dwDataTransfer &= !DATATRANSF_SEND;
			continue;
		}

		//the time here
		LARGE_INTEGER liCountFirst, liCountLast, liFreq;
		QueryPerformanceCounter(&liCountFirst);
		QueryPerformanceFrequency(&liFreq);

		if (itemType == ItemType_File)
		{
			SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Preparing to send the file...", 0);
			
			Send::wsChildFileName = Send::wsParentFileName;
			bool result = FileSender(pThis->m_dataTransferer, Send::wsParentFileName, *pThis->m_pFile, liSize.QuadPart, Send::bModeRepair, pThis->m_transferProgress)();
			if (false == result) 
			{
				return 0;
			}
		}
		else
		{
			SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Preparing to send the folder", 0);

			//all items will be relative to Send::wsParentFileName
			//nPos will point to the first character after Send::wsParentFileName
			int nPos = StringLen(Send::wsParentFileName) + 1;
	
			//we delete nr_del elements for each string
			CDoubleList<FILE_ITEM>::Iterator I;
			for (I = Items.begin(); I != NULL; I = I->pNext)
			{
				//the type of the item: file or folder
				if (false == pThis->m_dataTransferer.SendDataShort(&I->m_Value.type, sizeof(int))) return 0;
				//the length of the relative path
				WORD len = (WORD)StringLen(I->m_Value.wsFullName + nPos) + 1;
				if (false == pThis->m_dataTransferer.SendDataShort(&len, sizeof(WORD))) return 0;
				//the relative path
				if (false == pThis->m_dataTransferer.SendData(I->m_Value.wsFullName + nPos, len * 2)) return 0;
				//if it is a file, we send the file
				if (I->m_Value.type == ItemType_File)
				{
					bool result = FileSender(pThis->m_dataTransferer, I->m_Value.wsFullName, *pThis->m_pFile, I->m_Value.size, Send::bModeRepair, pThis->m_transferProgress)();
					if (!result) 
					{
						return 0;
					}
				}
			}
		}

		//clean up
		if (Send::File.IsOpened()) Send::File.Close();

		//enable again
		PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonSend, 1);
		PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hButtonBrowse, 1);

		//update the transfer flag
		dwDataTransfer &= !DATATRANSF_SEND;

		//finalize timer
		QueryPerformanceCounter(&liCountLast);
		double dbTime = (liCountLast.QuadPart - liCountFirst.QuadPart)/(double)liFreq.QuadPart;
		if (dbTime > ceil(dbTime))
		{
			dbTime = ceil(dbTime) + 1;
		}
		else dbTime = ceil(dbTime);

		pThis->m_transferProgress.EndBatch((DWORD)dbTime);
	}
	return 0;
}