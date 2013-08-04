#include "SendFilesThread.h"

#include "MainDlg.h"
#include "Application.h"
#include "String.h"
#include "DoubleList.h"
#include "Tools.h"
#include "FileSender.h"
#include "Debug.h"

void SendFilesThread::OnStart()
{
	//while it has not been ordered to end the program we continue to transfer data
	while (bOrderEnd == FALSE && Connected != Conn::NotConnected)
	{
		//if it was not pressed the Send button, we do a handshake
		if (false == m_dataTransferer.WaitForDataSend()) return /*0*/;

		//cleaning anything that was left:
		if (m_pFile->IsOpened()) m_pFile->Close();
		//we do not remove Send::wsParentFileName and Send::wsParentDisplayName because they were chosen in BROWSE and we need them NOW

		m_transferProgress.BeginBatch();
		m_bModeRepair = 0;

		PostMessage(MainDlg::m_hBarSend, PBM_SETPOS, 0, 0);
		PostMessage(theApp->GetMainWindow(), WM_ENABLECHILD, (WPARAM)MainDlg::m_hCheckRepairMode, 0);
		
		//SENDING THE ITEM TYPE
		if (false == m_dataTransferer.SendDataSimple(&m_itemType, sizeof(m_itemType))) return /*0*/;

		//ONLY IF itemType == ItemType_Folder
		if (m_itemType == ItemType_Folder)
		{
			//SENDING THE MODE: NORMAL/REPAIR
			if (BST_CHECKED == SendMessage(MainDlg::m_hCheckRepairMode, BM_GETCHECK, 0, 0))
				m_bModeRepair = TRUE;
			else m_bModeRepair = FALSE;

			if (false == m_dataTransferer.SendDataSimple(&m_bModeRepair, sizeof(m_bModeRepair))) return /*0*/;
		}

		//SENDING THE FILENAME LENGTH
		//getting the length from the string
		DWORD len = m_wsParentDisplayName.length();
		len++;

		if (false == m_dataTransferer.SendDataSimple(&len, sizeof(len))) return /*0*/;
		
		//SENDING THE FILENAME STRING
		if (false == m_dataTransferer.SendDataSimple((void*)m_wsParentDisplayName.data(), len * 2)) return /*0*/;

		int nCount = 0;
		LARGE_INTEGER liSize = {0};

		//TODO: need to load this value from somewhere!
		std::wstring wsParentFileName;

		//CALCULATING AND SENDING FILE/FILES SIZE
		CDoubleList<FILE_ITEM> Items(OnDestroyFileItemCoTask);
		if (m_itemType == ItemType_File)
		{
			
			if (false == CalcFileSize(wsParentFileName.data(), liSize)) 
			{
				MessageBox(0, L"And error has occured while trying to calculate the size of the specified file", 0, 0);
				continue;
			}
			if (false == m_dataTransferer.SendDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return /*0*/;
		}
		else
		{
			IShellFolder* pDesktop;
			HRESULT hr = SHGetDesktopFolder(&pDesktop);
			if (FAILED(hr))
			{
				DisplayError(hr);
				return /*false*/;
			}

			ITEMIDLIST* pidlFolder;
			hr = pDesktop->ParseDisplayName(theApp->GetMainWindow(), NULL, (LPWSTR)wsParentFileName.data(), NULL, &pidlFolder, NULL);
			if (FAILED(hr))
			{
				pDesktop->Release();
				DisplayError(hr);
				return /*false*/;
			}

			IShellFolder* pFolder;
			hr = pDesktop->BindToObject(pidlFolder, NULL, IID_IShellFolder, (void**)&pFolder);
			if (FAILED(hr))
			{
				pDesktop->Release();
				CoTaskMemFree(pidlFolder);

				DisplayError(hr);
				return /*false*/;
			}

			pDesktop->Release();
			CoTaskMemFree(pidlFolder);

			SearchFolder(pFolder, Items, liSize);
			pFolder->Release();
			nCount = Items.size();
			
			//WE NOW SEND THE nCount AND liSize DATA
			if (false == m_dataTransferer.SendDataSimple(&nCount, sizeof(nCount))) return /*0*/;
			if (false == m_dataTransferer.SendDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return /*0*/;
		}

		m_transferProgress.SetFileSize(liSize.QuadPart);

		//RECEIVING THE CONFIRMATION: whether the transfer is allowed or denied:
		if (m_itemType == ItemType_File)
			SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Asking your friend to receive the file...", 0);
		else
			SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Asking your friend to receive the folder...", 0);

		bool bConfirmed = 0;
		//we wait until the other user decides whether to save the item or to refuse it.
		if (false == m_dataTransferer.WaitForDataReceive()) return /*0*/;
		if (false == m_dataTransferer.ReceiveDataSimple(&bConfirmed, sizeof(bool))) {return /*0*/;}

		if (bConfirmed == 0) 
		{
			if (m_itemType == ItemType_File)
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

		if (m_itemType == ItemType_File)
		{
			SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Preparing to send the file...", 0);
			
			m_wsChildFileName = wsParentFileName;
			bool result = FileSender(m_dataTransferer, wsParentFileName, *m_pFile, liSize.QuadPart, m_bModeRepair, m_transferProgress)();
			if (false == result) 
			{
				return /*0*/;
			}
		}
		else
		{
			SendMessage(theApp->GetMainWindow(), WM_SETITEMTEXT, (WPARAM)L"Preparing to send the folder", 0);

			//all items will be relative to Send::wsParentFileName
			//nPos will point to the first character after Send::wsParentFileName
			int nPos = wsParentFileName.length() + 1;
	
			//we delete nr_del elements for each string
			CDoubleList<FILE_ITEM>::Iterator I;
			for (I = Items.begin(); I != NULL; I = I->pNext)
			{
				//the type of the item: file or folder
				if (false == m_dataTransferer.SendDataShort(&I->m_Value.type, sizeof(int))) return /*0*/;
				//the length of the relative path
				WORD len = (WORD)StringLen(I->m_Value.wsFullName + nPos) + 1;
				if (false == m_dataTransferer.SendDataShort(&len, sizeof(WORD))) return /*0*/;
				//the relative path
				if (false == m_dataTransferer.SendData(I->m_Value.wsFullName + nPos, len * 2)) return /*0*/;
				//if it is a file, we send the file
				if (I->m_Value.type == ItemType_File)
				{
					bool result = FileSender(m_dataTransferer, I->m_Value.wsFullName, *m_pFile, I->m_Value.size, m_bModeRepair, m_transferProgress)();
					if (!result) 
					{
						return /*0*/;
					}
				}
			}
		}

		//clean up
		if (m_pFile->IsOpened()) m_pFile->Close();

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

		m_transferProgress.EndBatch((DWORD)dbTime);
	}
	return /*0*/;
}