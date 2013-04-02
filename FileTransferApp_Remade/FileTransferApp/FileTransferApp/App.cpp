#include "App.h"
#include "resource.h"
#include "General.h"
#include "Exceptions.h"

#include "Recv.h"
#include "Send.h"
#include "Events.h"

App::App()
{
	m_hKey = NULL;
	m_hIcon = NULL;
	m_hInst = NULL;
	m_bIsMinimized = FALSE;
}

App::~App()
{
	if (m_hKey) RegCloseKey(m_hKey);
}

void App::Initialize()
{
	//setting the icon
	m_hIcon = LoadIcon(m_hInst, MAKEINTRESOURCE(IDR_MAINFRAME));
	SendMessage(GetHWND(), WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
	SendMessage(GetHWND(), WM_SETICON, ICON_SMALL, (LPARAM)m_hIcon);

	//finding out whether this operating system is >=Vista or not.
	OSVERSIONINFO osinfo = {0};
	osinfo.dwOSVersionInfoSize = sizeof(osinfo);
	GetSystemInfo((LPSYSTEM_INFO)&osinfo);
	BOOL bIsVista = osinfo.dwMajorVersion >= 6;

	HKEY hAux = NULL;
	m_hKey = NULL;

	//initialize registry: we retrieve the Nicknames and IPs and set in the combo-box
	//the last used IP address. We save m_hKey for further use.
	//m_hKey will be HKLM\Software\FeoggouApp\FileTransferApp
	try
	{
		WNE(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software", 0, KEY_READ, &m_hKey));

		hAux = m_hKey;
		WNE(RegCreateKeyExW(hAux, L"FeoggouApp", 0, 0, 0, KEY_READ | KEY_WRITE, 0, &m_hKey, 0));

		RegCloseKey(hAux);
		hAux = m_hKey;

		WNE(RegCreateKeyExW(hAux, L"FileTransferApp", 0, 0, 0, KEY_READ | KEY_WRITE, 0, &m_hKey, 0));
		RegCloseKey(hAux);
		hAux = 0;

		//now m_hKey is HKLM\Software\FeoggouApp\FileTransferApp
		WCHAR wsValueName[35] = {0}, wsData[100] = {0};
		int i = 0;

		DWORD dwValueNr = 35, dwDataNr = 100;
		LONG result;
		//retrieving all values & datas: values = nicks; data = IPs
		while (ERROR_SUCCESS == (result = RegEnumValueW(m_hKey, i, wsValueName, &dwValueNr, 0, 0, (BYTE*)wsData, &dwDataNr)))
		{
			if (i == 0)
			{
				//the first one is saved for further use
				SetWindowTextW(m_mainDlg.m_hComboNickIP, wsValueName);
			}

			//we don't add empty strings (i.e. the default value, if it has a data as null).
			if (*wsValueName)
				SendMessage(m_mainDlg.m_hComboNickIP, CB_ADDSTRING, 0, (LPARAM)wsValueName);

			dwValueNr = 35;
			dwDataNr = 100;
			i++;
		}
		if (result !=  ERROR_NO_MORE_ITEMS)
		{
			ThrowWin(result);
		}

		//retrieve the default value data
		DWORD dwError = RegQueryValueExW(m_hKey, 0, 0, 0, (BYTE*)wsData, &dwDataNr);
		if (0 != dwError && ERROR_FILE_NOT_FOUND != dwError)
		{
			ThrowWin(result);
		}

		//if there is a default value set, we set it here. (otherwise, the value already set remains)
		if (dwError != ERROR_FILE_NOT_FOUND && *wsData)
		{
			SetWindowTextW(m_mainDlg.m_hComboNickIP, wsData);
		}
	}

	catch (Exceptions::WindowException& e)
	{
		if (hAux) RegCloseKey(hAux);
		if (m_hKey) RegCloseKey(m_hKey);
		throw;
	}
}

void App::DestroyTrayIcon()
{
	if (m_bIsMinimized)
	{
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = GetHWND();
		nid.uID = SYSTRAY_ICON;
		Shell_NotifyIcon(NIM_DELETE, &nid);
	}
}

void App::CloseAll()
{
	//we need to close connection
	bOrderEnd = TRUE;
	//no data transfer is allowed anymore (this will deny Send::Thread and Recv::Thread from starting to transfer)
	dwDataTransfer = 0;

	//cleaning up
	//receive
	if (Recv::File.IsOpened()) Recv::File.Close();
	if (Recv::wsParentDisplayName) {delete[] Recv::wsParentDisplayName; Recv::wsParentDisplayName = 0;}
	if (Recv::wsChildFileName) {delete[] Recv::wsChildFileName; Recv::wsChildFileName = NULL;}

	//send
	if (Send::File.IsOpened()) Send::File.Close();
	if (Send::wsParentFileName) {delete[] Send::wsParentFileName; Send::wsParentFileName = NULL;}

	EnableWindow(m_mainDlg.m_hButtonSend, 0);
	SetWindowTextW(m_mainDlg.m_hEditItemPath, L"");

	//close the sockets BEFORE closing the threads!
	int nError;
	//if the socket was not already closed, we close it.
	//we delete the pointer after the threads have closed only.
	if (Send::pSocket)
	{
		nError = Send::pSocket->Close();
		if (nError)
		{
			DisplayError(nError);

			delete Send::pSocket;
			Send::pSocket = NULL;
			PostQuitMessage(-1);
		}
	}

	//if the socket was not already closed, we close it.
	//we delete the pointer after the threads have closed only.
	if (Recv::pSocket)
	{
		nError = Recv::pSocket->Close();
		if (nError)
		{
			DisplayError(nError);

			delete Recv::pSocket;
			Recv::pSocket = NULL;
			PostQuitMessage(-1);
		}
	}

	//these 2 must have been closed allready
	if (Recv::hConnThread != INVALID_HANDLE_VALUE)
	{
		//it closes itself after finishing
		WaitForSingleObject(Recv::hConnThread, INFINITE);

#ifdef _DEBUG
		//Recv::hConnThread should now be closed and set to INVALID_HANDLE_VALUE
		_ASSERTE(Recv::hConnThread == INVALID_HANDLE_VALUE);

		CloseHandle(Recv::hConnThread);
		Recv::hConnThread = INVALID_HANDLE_VALUE;
#endif
	}

	if (Send::hConnThread != INVALID_HANDLE_VALUE)
	{
		//it closes itself after finishing
		WaitForSingleObject(Send::hConnThread, INFINITE);

#ifdef _DEBUG
		//Send::hConnThread should now be closed and set to INVALID_HANDLE_VALUE
		CloseHandle(Send::hConnThread);
		Send::hConnThread = INVALID_HANDLE_VALUE;
#endif
	}

	//we wait for them, and then close them.
	//the sockets are already closed and bOrderEnd is set, so there should be no problem

	//first the Recv::hThread
	if (Recv::hThread != INVALID_HANDLE_VALUE)
	{
		while (WAIT_TIMEOUT == WaitForSingleObject(Recv::hThread, 100))
		{
			SleepEx(50, TRUE);
		}

		CloseHandle(Recv::hThread);
		Recv::hThread = INVALID_HANDLE_VALUE;
	}

	//now close the Send::hThread
	if (Send::hThread != INVALID_HANDLE_VALUE)
	{
		while (WAIT_TIMEOUT == WaitForSingleObject(Send::hThread, 100))
		{
			SleepEx(50, TRUE);
		}

		CloseHandle(Send::hThread);
		Send::hThread = INVALID_HANDLE_VALUE;
	}

	//We have closed the threads, so it's safe to delete the socket pointers
	if (Send::pSocket)
	{
		delete Send::pSocket;
		Send::pSocket = NULL;
	}

	if (Recv::pSocket)
	{
		delete Recv::pSocket;
		Recv::pSocket = NULL;
	}

	//update the UI: it is possible that we do not close the program now.
	m_mainDlg.UpdateUIDisconnected();
}

void App::OnNetworkEvent(const Events::NetworkEvent& anEvent)
{
	typedef Events::NetworkEvent::EventHappened Event;
	Event theEvent = anEvent.WhatHappened();

	switch (theEvent)
	{
	case Event::ConnGracefullyClosed:
		{
			m_networkManager.OnCloseNetwork();
			m_mainDlg.UpdateUI(MainDlg::UIEvent::OtherComputerClosedConn);
		}
		break;

	default:
		{
		}
	}
}