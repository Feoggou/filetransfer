#include "Send.h"
#include "FileTransferDlg.h"
#include "SocketServer.h"
#include "CRC.h"

#include <math.h>

extern HWND hMainWnd;

HANDLE Send::hThread = INVALID_HANDLE_VALUE;
HANDLE Send::hConnThread = INVALID_HANDLE_VALUE;

CSourceFile Send::File;
DWORD Send::dwCurrentPartGlobal = 0;
BOOL Send::bModeRepair = false;
DWORD Send::dwNrGreatParts = 0;
WCHAR Send::wsTotalSize[20];

CSamSocket* Send::pSocket = NULL;
ItemType Send::itemType;

WCHAR* Send::wsParentFileName = NULL;
WCHAR* Send::wsParentFileDisplayName = NULL;
WCHAR* Send::wsChildFileName = NULL;

inline BOOL Send::SendData(void* Buffer, int dSize)
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
				SendMessage(CFileTransferDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(hMainWnd, WM_CLOSECONNECTION, 0, 0);
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

inline BOOL Send::ReceiveData(void* Buffer, int dSize)
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
				SendMessage(CFileTransferDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(hMainWnd, WM_CLOSECONNECTION, 0, 0);
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
				SendMessage(CFileTransferDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(hMainWnd, WM_CLOSECONNECTION, 0, 0);
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
			if (dSentRec == -1)
			{
				if (bOrderEnd) return false;
				continue;
			}
		}
		else
		{
			dwIsOK = 0;
			pSocket->Send(&dwIsOK, sizeof(dwIsOK), dSentRec);
			if (dSentRec == -1)
			{
				if (bOrderEnd) return false;
				continue;
			}
		}

	//if validation failed try again!
	} while (dwIsOK != 1);

	return true;
}

inline BOOL Send::SendDataSimple(void* Buffer, int dSize)
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

inline BOOL Send::ReceiveDataSimple(void* Buffer, int dSize)
{
	_ASSERTE(Buffer);
	int dSentRec;
	
	//receive data
try_again:
	pSocket->Receive(Buffer, dSize, dSentRec);
	if (dSentRec <= 0)
	{
		if (bOrderEnd) return false;
		if (0 == dSentRec)
		{
			SendMessage(CFileTransferDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
			PostMessage(hMainWnd, WM_CLOSECONNECTION, 0, 0);
			return false;
		}

		else if (dSentRec == -1)
		{
			goto try_again;
		}
	}

	return true;
}

inline BOOL Send::SendDataShort(void* Buffer, int dSize)
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
				SendMessage(CFileTransferDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(hMainWnd, WM_CLOSECONNECTION, 0, 0);
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
	}

	//if validation failed try again!
	while (dwIsOK != 1);

	return true;
}

inline BOOL Send::ReceiveDataShort(void* Buffer, int dSize)
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
				SendMessage(CFileTransferDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(hMainWnd, WM_CLOSECONNECTION, 0, 0);
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
				SendMessage(CFileTransferDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(hMainWnd, WM_CLOSECONNECTION, 0, 0);
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

inline BOOL Send::WaitForDataSend()
{
	int dSentRec;
	BYTE aux;
	BYTE dIsReady = 0;

	do
	{
		if (bOrderEnd) return false;
		Sleep(500);
		dIsReady = dwDataTransfer & DATATRANSF_SEND;

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
				SendMessage(CFileTransferDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(hMainWnd, WM_CLOSECONNECTION, 0, 0);
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

inline BOOL Send::WaitForDataReceive()
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
				SendMessage(CFileTransferDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
				PostMessage(hMainWnd, WM_CLOSECONNECTION, 0, 0);
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

BOOL Send::SendOneFile(WCHAR* wsReadFile, LONGLONG& llSize)
{
	//now, we open the file and send it to save
	if (FALSE == File.Open(wsReadFile, &llSize)) return false;

	Send::wsChildFileName = wsReadFile;
	DWORD nrParts = 0;
	{
		SendDataShort(&llSize, sizeof(LONGLONG));
		nrParts = (DWORD)llSize / BLOCKSIZE;
		if (llSize % BLOCKSIZE) nrParts++;
	}

	if (bModeRepair)
	{
		//we need confirmation: whether this file is ok or not
		BYTE exists;
		if (false == ReceiveDataShort(&exists, sizeof(BYTE))) return false;
		//if the file is ok, we skip it:
		if (1 == exists) 
		{
			Send::File.Close(); 
			Send::dwCurrentPartGlobal += nrParts;
			if (!CFileTransferDlg::m_bIsMinimized)
			{
				SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)wsReadFile, 1);
				int oldpos = Send::dwCurrentPartGlobal * 100 / Send::dwNrGreatParts;
				PostMessage(CFileTransferDlg::m_hBarSend, PBM_SETPOS, oldpos, 0);
			}
			return true;
		}
	}

	//we send all but the last piece:
	DWORD oldpos = 0;
	WCHAR wsMessage[300];

	LARGE_INTEGER last_count, curr_count, freq;
	QueryPerformanceCounter(&last_count);
	QueryPerformanceFrequency(&freq);

	WCHAR wsTimeLeft[20];
	StringCopy(wsTimeLeft, L"unknown");

	DWORD lasti = 0, deltai = 0;
	float deltax = 0, delta = 0;
	float speed = 0;
	
	WCHAR wsSpeed[15];
	BOOL bGotInside = FALSE;

	DWORD dwRead;

	//transfering the file
	for (DWORD i = 1; i < nrParts; i++)
	{
		if (bOrderEnd) return false;

		//reading from the file
		if (false == Send::File.ReadBlock(dwRead)) return false;

		//SENDING DATA
		if (false == SendData(Send::File.m_pCurrentPos, dwRead)) return false;

		dwCurrentPartGlobal++;

		//we must update the UI
		QueryPerformanceCounter(&curr_count);

		if (!CFileTransferDlg::m_bIsMinimized)
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
				FormatTime(wsTimeLeft, (DWORD)(ceil((deltax/(float)deltai) * (dwNrGreatParts - dwCurrentPartGlobal + 1))));
				lasti = i;
			
				//oldpos = pozitia in bara de progress, se actualizeaza aprox. o data pe secunda.
				//oldpos = x, din x% (x = 1->100) finished all.
				oldpos = dwCurrentPartGlobal * 100 / dwNrGreatParts;
				PostMessage(CFileTransferDlg::m_hBarSend, PBM_SETPOS, oldpos, 0);
				//viteza = nr de partzi care s-au transferat in timpul deltax * catzi bytes are fiecare parte / deltax
				if (deltai) speed = deltai * BLOCKSIZE / deltax;
				else {speed = 0;}

				SpeedFtoString(speed, wsSpeed);
				StringFormat(wsMessage, L"%d%% of %s; Speed: %s; Time Left: %s", oldpos, wsTotalSize, wsSpeed, wsTimeLeft);
				SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)wsMessage, 0);

				SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)wsReadFile, 1);

				deltax = 0;
			}
		}

		last_count = curr_count;
		
	}

	//now, send the last piece. we do not know how large it is, so we have to read its size first.
	if (false == File.ReadBlock(dwRead)) return false;

	//NOW WE TRANSFER THE SIZE OF THE LAST PIECE
	if (false == SendDataShort(&dwRead, sizeof(DWORD))) return false;

	//NOW WE TRANSFER THE LAST PIECE
	if (false == SendData(Send::File.m_pCurrentPos, dwRead)) return false;

	Send::File.Close();

	dwCurrentPartGlobal++;

	if (!CFileTransferDlg::m_bIsMinimized)
	{
		//we update the user interface
		oldpos = dwCurrentPartGlobal * 100 / dwNrGreatParts;
		if (!bGotInside)
		{
			speed = nrParts * BLOCKSIZE / deltax;
			SpeedFtoString(speed, wsSpeed);
			FormatTime(wsTimeLeft, (DWORD)(ceil((deltax/(float)nrParts) * (dwNrGreatParts - dwCurrentPartGlobal + 1))));
		}
		StringFormat(wsMessage, L"%d%% of %s; Speed: %s; Time Left: %s", oldpos, wsTotalSize, wsSpeed , wsTimeLeft);
		SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)wsMessage, 0);

		SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)wsReadFile, 1);
		PostMessage(CFileTransferDlg::m_hBarSend, PBM_SETPOS, oldpos, 0);
	}
	return true;
}

DWORD Send::ThreadProc(void)
{
	//while it has not been ordered to end the program we continue to transfer data
	while (bOrderEnd == FALSE && Connected != Conn::NotConnected)
	{
		//if it was not pressed the Send button, we do a handshake
		if (false == WaitForDataSend()) return 0;

		//cleaning anything that was left:
		if (Send::File.IsOpened()) Send::File.Close();
		//we do not remove Send::wsParentFileName and Send::wsParentDisplayName because they were chosen in BROWSE and we need them NOW
		Send::dwCurrentPartGlobal = 0;
		Send::bModeRepair = 0;
		Send::dwNrGreatParts = 0;

		PostMessage(CFileTransferDlg::m_hBarSend, PBM_SETPOS, 0, 0);
		PostMessage(hMainWnd, WM_ENABLECHILD, (WPARAM)CFileTransferDlg::m_hCheckRepairMode, 0);
		
		//SENDING THE ITEM TYPE
		if (false == SendDataSimple(&Send::itemType, sizeof(Send::itemType))) return 0;

		//ONLY IF itemType == ItemType::Folder
		if (Send::itemType == ItemType::Folder)
		{
			//SENDING THE MODE: NORMAL/REPAIR
			if (BST_CHECKED == SendMessage(CFileTransferDlg::m_hCheckRepairMode, BM_GETCHECK, 0, 0))
				Send::bModeRepair = TRUE;
			else Send::bModeRepair = FALSE;

			if (false == SendDataSimple(&Send::bModeRepair, sizeof(Send::bModeRepair))) return 0;
		}

		//SENDING THE FILENAME LENGTH
		//getting the length from the string
		DWORD len = StringLen(Send::wsParentFileDisplayName);
		len++;

		if (false == SendDataSimple(&len, sizeof(len))) return 0;
		
		//SENDING THE FILENAME STRING
		if (false == SendDataSimple(Send::wsParentFileDisplayName, len * 2)) return 0;

		int nCount = 0;
		LARGE_INTEGER liSize = {0};

		//CALCULATING AND SENDING FILE/FILES SIZE
		CDoubleList<FILE_ITEM> Items(OnDestroyFileItemCoTask);
		if (Send::itemType == ItemType::File)
		{
			if (false == CalcFileSize(Send::wsParentFileName, liSize)) 
			{
				MessageBox(0, L"And error has occured while trying to calculate the size of the specified file", 0, 0);
				continue;
			}
			if (false == SendDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return 0;
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
			hr = pDesktop->ParseDisplayName(hMainWnd, NULL, Send::wsParentFileName, NULL, &pidlFolder, NULL);
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
			if (false == SendDataSimple(&nCount, sizeof(nCount))) return 0;
			if (false == SendDataSimple(&liSize.QuadPart, sizeof(liSize.QuadPart))) return 0;
		}

		SizeLItoString(liSize, Send::wsTotalSize);
		Send::dwNrGreatParts = (DWORD) liSize.QuadPart / BLOCKSIZE;
		if (liSize.QuadPart % BLOCKSIZE) Send::dwNrGreatParts++;

		//RECEIVING THE CONFIRMATION: whether the transfer is allowed or denied:
		if (Send::itemType == ItemType::File)
			SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)L"Asking your friend to receive the file...", 0);
		else
			SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)L"Asking your friend to receive the folder...", 0);

		bool bConfirmed = 0;
		//we wait until the other user decides whether to save the item or to refuse it.
		if (false == WaitForDataReceive()) return 0;
		if (false == ReceiveDataSimple(&bConfirmed, sizeof(bool))) {return 0;}

		if (bConfirmed == 0) 
		{
			if (Send::itemType == ItemType::File)
				SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)L"The file has been refused...", 0);
			else
				SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)L"The folder has been refused...", 0);

			//WE MUST ALLOW BROWSE AND SEND AND REPAIR!!
			PostMessage(hMainWnd, WM_ENABLECHILD, (WPARAM)CFileTransferDlg::m_hButtonSend, 1);
			PostMessage(hMainWnd, WM_ENABLECHILD, (WPARAM)CFileTransferDlg::m_hButtonBrowse, 1);
			PostMessage(hMainWnd, WM_ENABLECHILD, (WPARAM)CFileTransferDlg::m_hCheckRepairMode, 1);
			dwDataTransfer &= !DATATRANSF_SEND;
			continue;
		}

		//the time here
		LARGE_INTEGER liCountFirst, liCountLast, liFreq;
		QueryPerformanceCounter(&liCountFirst);
		QueryPerformanceFrequency(&liFreq);

		if (itemType == ItemType::File)
		{
			SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)L"Preparing to send the file...", 0);

			if (false == SendOneFile(Send::wsParentFileName, liSize.QuadPart)) 
			{
				return 0;
			}
		}
		else
		{
			SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)L"Preparing to send the folder", 0);

			//all items will be relative to Send::wsParentFileName
			//nPos will point to the first character after Send::wsParentFileName
			int nPos = StringLen(Send::wsParentFileName) + 1;
	
			//we delete nr_del elements for each string
			CDoubleList<FILE_ITEM>::Iterator I;
			for (I = Items.begin(); I != NULL; I = I->pNext)
			{
				//the type of the item: file or folder
				if (false == SendDataShort(&I->m_Value.type, sizeof(int))) return 0;
				//the length of the relative path
				WORD len = (WORD)StringLen(I->m_Value.wsFullName + nPos) + 1;
				if (false == SendDataShort(&len, sizeof(WORD))) return 0;
				//the relative path
				if (false == SendData(I->m_Value.wsFullName + nPos, len * 2)) return 0;
				//if it is a file, we send the file
				if (I->m_Value.type == ItemType::File)
				{
					if (false == SendOneFile(I->m_Value.wsFullName, I->m_Value.size)) 
					{
						return 0;
					}
				}
			}
		}

		//clean up
		if (Send::File.IsOpened()) Send::File.Close();

		//enable again
		PostMessage(hMainWnd, WM_ENABLECHILD, (WPARAM)CFileTransferDlg::m_hButtonSend, 1);
		PostMessage(hMainWnd, WM_ENABLECHILD, (WPARAM)CFileTransferDlg::m_hButtonBrowse, 1);

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

		WCHAR wsTime[20];
		FormatTime(wsTime, (DWORD)dbTime);

		//we update the user interface
		WCHAR wsMessage[300];
		StringFormat(wsMessage, L"100%% of %s; Speed: 0 KB/s; Time Left: Finished!", Send::wsTotalSize);
		SendMessage(hMainWnd, WM_SETITEMTEXT, (WPARAM)wsMessage, 0);
		SendMessage(CFileTransferDlg::m_hBarSend, PBM_SETPOS, 100, 0);

		if (Send::itemType == ItemType::File)
		{
			StringFormatW(wsMessage, L"The file has been transfered successfully in %s!", wsTime);
			//we post this message, because the UI thread must perform it and this thread must continue
			//sending keep-alive messages
			PostMessage(hMainWnd, WM_SHOWMESSAGEBOX, 0, (LPARAM)wsMessage);
		}
		else
		{
			StringFormatW(wsMessage, L"The folder has been transfered successfully in %s!", wsTime);
			//we post this message, because the UI thread must perform it and this thread must continue
			//sending keep-alive messages
			PostMessage(hMainWnd, WM_SHOWMESSAGEBOX, 0, (LPARAM)wsMessage);
		}
	}
	return 0;
}

#include "Recv.h"

DWORD Send::ConnThreadProc(void)
{
	bOrderEnd = false;
	int nError;

	Send::pSocket = new CSocketServer();
	Recv::pSocket = new CSocketServer();
	CSocketServer* pRecvServer = (CSocketServer*)Recv::pSocket;
	CSocketServer* pSendServer = (CSocketServer*)Send::pSocket;

	nError = pRecvServer->Create(14147);
	if (nError)
	{
		DisplayError(nError);
		goto final;
	}

	nError = pSendServer->Create(14148);
	if (nError)
	{
		DisplayError(nError);
		goto final;
	}

	//Listen
	nError = pRecvServer->Listen();
	if (nError)
	{
		DisplayError(nError);
		goto final;
	}

	nError = pSendServer->Listen();
	if (nError)
	{
		DisplayError(nError);
		goto final;
	}

	SendMessage(CFileTransferDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The Server is turned on and waiting for the client to connect.");

	//Accept
	if (!bOrderEnd)
	{
		nError = pRecvServer->Accept();
		if (nError && !bOrderEnd)
		{
			DisplayError(nError);
			goto final;
		}
	}

	nError = pSendServer->Accept();
	if (nError && !bOrderEnd)
	{
		DisplayError(nError);
		goto final;
	}

final:
	CloseHandle(Send::hConnThread);
	Send::hConnThread = INVALID_HANDLE_VALUE;
	Connected = Conn::ConnAsServer;
	
	if (!bOrderEnd)
	{
		Recv::hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Recv::ThreadProc, hMainWnd, 0, 0);
		Send::hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Send::ThreadProc, hMainWnd, 0, 0);

		//after the connection is successful:
		SendMessage(CFileTransferDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"Connection to the client has been established.");
		PostMessage(hMainWnd, WM_ENABLECHILD, (WPARAM)CFileTransferDlg::m_hButtonBrowse, 1);
	}
	
	return 0;
}