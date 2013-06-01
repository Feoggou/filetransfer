#include "MainDlg.h"
#include "resource.h"
#include "AboutDlg.h"

#include "Application.h"
#include "Tools.h"

#include "Recv.h"
#include "Send.h"

#include "NickNameDlg.h"

//#include <cmath>

#include "SocketClient.h"

#define IDMI_ABOUT			0x0010

#define SYSTRAY_ICON		1000
#define WM_TRAYMSG			WM_USER + 4

#define ID_RESTOREPROG		10001
#define ID_EXITPROG			10002

HWND MainDlg::m_hCheckRepairMode = NULL;
HWND MainDlg::m_hStatusText = NULL;
HWND MainDlg::m_hButtonConnect = NULL;
HWND MainDlg::m_hButtonCreateConn = NULL;
HWND MainDlg::m_hButtonSend = NULL;
HWND MainDlg::m_hButtonBrowse = NULL;
HWND MainDlg::m_hEditItemPath = NULL;

HWND MainDlg::m_hStatusSend = NULL;
HWND MainDlg::m_hStatusRecv = NULL;
HWND MainDlg::m_hStatusCurrFileSend = NULL;
HWND MainDlg::m_hStatusCurrFileRecv = NULL;
HWND MainDlg::m_hComboNickIP = NULL;

HWND MainDlg::m_hBarSend = NULL;
HWND MainDlg::m_hBarRecv = NULL;

BOOL MainDlg::m_bIsMinimized = NULL;

void MainDlg::OnClose()
{
	if (dwDataTransfer)
	{
		if (MessageBox(m_hWnd, L"File transfer is not finished. Are you sure you want to cancel the transfer?",
			L"File Transfer cancelling", MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
			return;
	}

	DestroyTrayIcon();
	CloseAll();

	Dialog::OnClose();
}

void MainDlg::OnSysCommand(WPARAM cmd, int x, int y) 
{
	if ((cmd & 0xFFF0) == IDMI_ABOUT) {
		AboutDlg dlgAbout(m_hWnd);
		dlgAbout.CreateModal();
	}
}

void MainDlg::OnAbout()
{
	AboutDlg dlgAbout(m_hWnd);
	dlgAbout.CreateModal();
}

void MainDlg::OnButtonSend()
{
	//get ready the filename
	SetDlgItemTextW(m_hWnd, IDC_ST_SEND, L"Not Sending");

	//use the thread for sending the data
	dwDataTransfer |= DATATRANSF_SEND;

	//while sending, we disable the "Send" and the "Browse" buttons
	EnableWindow(m_hButtonSend, false);
	EnableWindow(m_hButtonBrowse, false);
}

void MainDlg::RestoreFromTray()
{
	DestroyTrayIcon();
	ShowWindow(m_hWnd, 1);
	m_bIsMinimized = FALSE;
}

void MainDlg::OnCommand(WORD source, WORD id, HWND /*hControl*/) 
{
	switch (id)
	{
	case IDC_ABOUT: OnAbout(); break;
	case IDC_BUTTON_SEND: OnButtonSend(); break;
	case IDC_BUTTON_CONNECT: OnButtonConnect(); break;
	case IDC_BUTTON_CREATE_CONNECTION: CreateConnection(); break;
	case IDC_BUTTON_BROWSE:	OnBrowse(); break;
	case IDC_EXIT: OnClose(); break;
		//the user has chosen the "Exit" item from the system tray menu.
		//we execute the same "Close" command
	case ID_EXITPROG: OnClose(); break;

		//restore the program from "minimized to tray" state.
		//the item menu is "Show"
	case ID_RESTOREPROG: if (source == MessageSource_Menu) RestoreFromTray(); break;
	}
}

void MainDlg::OnEnableChild(HWND hChild, bool enable)
{
	EnableWindow(hChild, enable);
}

void MainDlg::OnSetItemText(LPARAM lParam, LPCWSTR text)
{
	switch (lParam)
	{
	case 0: SetWindowText(m_hStatusSend, text); break;
	case 1: SetWindowText(m_hStatusCurrFileSend, text); break;
	case 2: SetWindowText(m_hStatusRecv, text); break;
	case 3: SetWindowText(m_hStatusCurrFileRecv, text); break;
	}
}

void MainDlg::OnTrayMessage(LPARAM lParam)
{
	if (lParam == WM_RBUTTONUP)
	{	
		HMENU hMenu = CreatePopupMenu();
		AppendMenuW(hMenu, MF_STRING, ID_RESTOREPROG, L"&Show");
		AppendMenuW(hMenu, MF_SEPARATOR, 0, 0);
		AppendMenuW(hMenu, MF_STRING, ID_EXITPROG, L"E&xit");

		POINT point;
		GetCursorPos(&point);
		TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, 0, m_hWnd, 0);
	}
}

void MainDlg::OnMinimized()
{
	_ASSERT(!m_bIsMinimized);
	m_bIsMinimized = TRUE;

	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = m_hWnd;
	nid.uID = SYSTRAY_ICON;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_TRAYMSG;
	nid.hIcon = m_hIcon;
	StringCopyW(nid.szTip, L"File Transfer Application");

	Shell_NotifyIcon(NIM_ADD, &nid);
	ShowWindow(m_hWnd, 0);
}

void MainDlg::OnSize(WPARAM type, int new_width, int new_height)
{
	//when the user has chosen to minimize the dialogbox, we actually hide it
	//and display an icon in the system tray.
	if (type == SIZE_MINIMIZED)
	{
		OnMinimized();
	}
}

void MainDlg::OnShowMessageBox(LPCWSTR message, LPCWSTR path)
{
	//if it is the receiver:
	if (path)
	{
		if (MessageBoxW(m_hWnd, message, L"Transfer finished", MB_YESNO) == IDYES)
		{
			ShellExecute(0, L"explore", path, NULL, NULL, SW_SHOWNORMAL);
		}

		delete[] message;
		delete[] path;

		//we finally reset the status text and progressbar.
		SendMessage(m_hWnd, WM_SETITEMTEXT, (WPARAM)L"Not Receiving", 2);
		SendMessage(m_hWnd, WM_SETITEMTEXT, (WPARAM)L"none", 3);
		SendMessage(MainDlg::m_hBarRecv, PBM_SETPOS, 0, 0);
	}

	//if it is the sender:
	else
	{
		MessageBoxW(m_hWnd, message, L"Transfer finished!", MB_ICONINFORMATION);

		//we reset the progressbar only after
		SendMessage(m_hWnd, WM_SETITEMTEXT, (WPARAM)L"Not Sending", 0);
		SendMessage(m_hWnd, WM_SETITEMTEXT, (WPARAM)L"none", 1);
		SendMessage(MainDlg::m_hBarSend, PBM_SETPOS, 0, 0);
		SendMessage(MainDlg::m_hCheckRepairMode, BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessage(m_hWnd, WM_ENABLECHILD, (WPARAM)MainDlg::m_hCheckRepairMode, 1);
	}
}

INT_PTR MainDlg::OnDialogProcedure(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		//WPARAM - HWND of the child
		//LPARAM - 1: enable; 0: disable
	case WM_ENABLECHILD: OnEnableChild((HWND)wParam, (BOOL)lParam); break;
		//LPARAM: 0 - for send info; 1 - for send curfile; 
		//        2 - for recv info; 3 - for recv curfile;
		//WPARAM: text
	case WM_SETITEMTEXT: OnSetItemText(lParam, LPCWSTR(wParam)); break;
		//closes the sockets, the threads, updates the UI after disconnection, deletes buffers
		//resets variables
	case WM_CLOSECONNECTION: CloseAll(); break;
		//when the user has right-clicked on the system tray icon of the program
		//create and display the menu
	case WM_TRAYMSG: OnTrayMessage(lParam); break;
	case WM_SIZE: OnSize(wParam, LOWORD(lParam), HIWORD(lParam)); break;
		//shows a messagebox (so that it is called by the UI thread)
		//lParam = the text; wParam = the style.
	case WM_SHOWMESSAGEBOX: OnShowMessageBox((LPCWSTR)lParam, (LPCWSTR)wParam); break;
	}

	return 0;
}

void MainDlg::OnInitDialog()
{
	//appending the "About" item in the system menu
	{
		HMENU hMenu = GetSystemMenu(m_hWnd, FALSE);
		int nID = GetMenuItemCount(hMenu);
		MENUITEMINFO mii = {0};
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID;
		mii.fType = MFT_STRING;
		mii.wID = IDMI_ABOUT;
		mii.dwTypeData = L"&About";

		InsertMenuItemW(hMenu, nID - 2, TRUE, &mii);
	}

	//setting the icon
	m_hIcon = LoadIcon(Application::GetHInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
	SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
	SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIcon);

	//initializing handles and controls: here the progress bars
	m_hBarRecv = GetDlgItem(m_hWnd, IDC_PROG_RECV);
	m_hBarSend = GetDlgItem(m_hWnd, IDC_PROG_SEND);

	//set the range of the progress bars
	SendMessageW(m_hBarRecv, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessageW(m_hBarSend, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

	//initializing handles and controls
	m_hCheckRepairMode = GetDlgItem(m_hWnd, IDC_CHK_REPAIRMODE);
	m_hStatusText = GetDlgItem(m_hWnd, IDC_STATUS);
	m_hButtonConnect = GetDlgItem(m_hWnd, IDC_BUTTON_CONNECT);
	m_hButtonCreateConn = GetDlgItem(m_hWnd, IDC_BUTTON_CREATE_CONNECTION);
	m_hComboNickIP = GetDlgItem(m_hWnd, IDC_COMBO_IP);
	m_hButtonSend = GetDlgItem(m_hWnd, IDC_BUTTON_SEND);
	m_hButtonBrowse = GetDlgItem(m_hWnd, IDC_BUTTON_BROWSE);
	m_hEditItemPath = GetDlgItem(m_hWnd, IDC_EDIT_BROWSE);

	m_hStatusSend = GetDlgItem(m_hWnd, IDC_ST_SEND);
	m_hStatusRecv = GetDlgItem(m_hWnd, IDC_ST_RECV);
	m_hStatusCurrFileSend = GetDlgItem(m_hWnd, IDC_ST_CURFILE_S);
	m_hStatusCurrFileRecv = GetDlgItem(m_hWnd, IDC_ST_CURFILE_R);

	//initialize registry: we retrieve the Nicknames and IPs and set in the combo-box
	//the last used IP address. We save m_hKey for further use.
	//m_hKey will be HKLM\Software\FeoggouApp\FileTransferApp
	DWORD dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software", 0, KEY_READ, &m_hKey);
	if (dwError != 0)
	{
		DisplayError(dwError);
		PostQuitMessage(-1);
		return;
	}

	HKEY hAux = m_hKey;
	dwError = RegCreateKeyExW(hAux, L"FeoggouApp", 0, 0, 0, KEY_READ | KEY_WRITE, 0, &m_hKey, 0);
	if (dwError != 0)
	{
		RegCloseKey(hAux);

		DisplayError(dwError);
		PostQuitMessage(-1);
		return;
	}
	RegCloseKey(hAux);
	hAux = m_hKey;

	dwError = RegCreateKeyExW(hAux, L"FileTransferApp", 0, 0, 0, KEY_READ | KEY_WRITE, 0, &m_hKey, 0);
	if (dwError != 0)
	{
		RegCloseKey(hAux);

		DisplayError(dwError);
		PostQuitMessage(-1);
		return;
	}
	RegCloseKey(hAux);

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
			SetWindowTextW(m_hComboNickIP, wsValueName);
		}

		//we don't add empty strings (i.e. the default value, if it has a data as null).
		if (*wsValueName)
			SendMessage(m_hComboNickIP, CB_ADDSTRING, 0, (LPARAM)wsValueName);

		dwValueNr = 35;
		dwDataNr = 100;
		i++;
	}
	if (result !=  ERROR_NO_MORE_ITEMS)
	{
		DisplayError(result);
		PostQuitMessage(-1);
		return;
	}

	//retrieve the default value data
	dwError = RegQueryValueExW(m_hKey, 0, 0, 0, (BYTE*)wsData, &dwDataNr);
	if (0 != dwError && ERROR_FILE_NOT_FOUND != dwError)
	{
		RegCloseKey(hAux);

		DisplayError(dwError);
		PostQuitMessage(-1);
		return;
	}

	//if there is a default value set, we set it here. (otherwise, the value already set remains)
	if (dwError != ERROR_FILE_NOT_FOUND && *wsData)
	{
		SetWindowTextW(m_hComboNickIP, wsData);
	}
}

void MainDlg::DestroyTrayIcon()
{
	if (m_bIsMinimized)
	{
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = m_hWnd;
		nid.uID = SYSTRAY_ICON;
		Shell_NotifyIcon(NIM_DELETE, &nid);
	}
}

void MainDlg::OnBrowse()
{
	//we create the menu
	RECT rect;
	GetWindowRect(m_hButtonBrowse, &rect);

	HMENU hMenu = CreatePopupMenu();
	AppendMenuW(hMenu, MF_STRING, 1, L"for a file");
	AppendMenuW(hMenu, MF_STRING, 2, L"for a folder");
	
	//displays a menu, so that the user would chose to pick a file or a folder
	int result = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTBUTTON | TPM_NOANIMATION, rect.left, rect.bottom, 0, m_hWnd, 0);

	if (result)
	{
		if (Send::File.IsOpened())
				Send::File.Close();
	}

	switch (result)
	{
	case 1://FILE
		{
			PickFile(m_hWnd);
		}
		break;

	case 2://FOLDER
		{
			PickFolder(m_hWnd);
		}
		break;
	}
}

#if _WIN32_WINNT == 0x0600
void MainDlg::PickFileVista(HWND hDlg)
{
	//create a IFileOpenDialog object
	IFileOpenDialog* pDlg;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&pDlg);
	if (FAILED(hr))
	{
		DisplayError(hr);
		return;
	}

	//we need the ITEMIDLIST of the desktop so we would create a shell item from it
	ITEMIDLIST* pidlDesktop;
	hr = SHGetKnownFolderIDList(FOLDERID_Desktop, 0, 0, &pidlDesktop);
	if (FAILED(hr))
	{
		pDlg->Release();

		DisplayError(hr);
		return;
	}

	//create the shell item
	IShellItem* pShellItem;
	hr = SHCreateItemFromIDList(pidlDesktop, IID_IShellItem, (void**)&pShellItem);
	if (FAILED(hr))
	{
		pDlg->Release();
		CoTaskMemFree(pidlDesktop);

		DisplayError(hr);
		return;
	}

	//we don't need the ITEMIDLIST anymore, so we free it.
	CoTaskMemFree(pidlDesktop);

	//set the default folder: the desktop
	pDlg->SetDefaultFolder(pShellItem);
	pDlg->SetOptions(FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST); 

	//show the dialogbox
	hr = pDlg->Show(m_hWnd);
	//if error or canceled
	if (FAILED(hr))
	{
		pShellItem->Release();
		pDlg->Release();

		if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
			DisplayError(hr);
		return;
	}
	//we don't need the desktop shell item, so we release it
	pShellItem->Release();

	//we store the result of the dialogbox in the shell item.
	hr = pDlg->GetResult(&pShellItem);
	if (FAILED(hr))
	{
		pDlg->Release();

		DisplayError(hr);
		return;
	}

	//we retrieve the displayname of the chosen item.
	WCHAR* wsFileName;
	pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &wsFileName);

	//we don't need the shell item retrieved and we don't need the dialogbox.
	pShellItem->Release();
	pDlg->Release();

	//saving info regarding the chosen item (file/folder)
	Send::itemType = ItemType::File;

	int len = StringLen(wsFileName);
	len++;

	//we store the filename in Send::wsParentFileName
	if (Send::wsParentFileName) delete[] Send::wsParentFileName;
	Send::wsParentFileName = new WCHAR[len];
	StringCopy(Send::wsParentFileName, wsFileName);
	//we don't need wsFileName any more
	CoTaskMemFree(wsFileName);

	//the wsParentFileDisplayName SHOULD NOT BE DELETED: it's only a pointer within wsParentFileName
	Send::wsParentFileDisplayName = StringRevChar(Send::wsParentFileName, '\\');
	Send::wsParentFileDisplayName++;

	//ok, we display the full file name in the editbox and enable the "Send" and "Repair" buttons
	SetWindowTextW(m_hEditItemPath, Send::wsParentFileName);
	EnableWindow(m_hButtonSend, true);
	EnableWindow(m_hCheckRepairMode, false);
}

void MainDlg::PickFolderVista(HWND hDlg)
{
	//we create a IFileOpenDialog object
	IFileOpenDialog* pDlg;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&pDlg);
	if (FAILED(hr))
	{
		DisplayError(hr);
		return;
	}

	//we retrieve the ITEMIDLIST of the desktop, so we would get its shell item
	ITEMIDLIST* pidlDesktop;
	hr = SHGetKnownFolderIDList(FOLDERID_Desktop, 0, 0, &pidlDesktop);
	if (FAILED(hr))
	{
		pDlg->Release();

		DisplayError(hr);
		return;
	}

	//we retrieve the shell item of the desktop
	IShellItem* pShellItem;
	hr = SHCreateItemFromIDList(pidlDesktop, IID_IShellItem, (void**)&pShellItem);
	if (FAILED(hr))
	{
		pDlg->Release();
		CoTaskMemFree(pidlDesktop);

		DisplayError(hr);
		return;
	}

	//ok, we don't need the ITEMIDLIST anymore, so we free it.
	CoTaskMemFree(pidlDesktop);

	//we set the desktop as the default directory.
	pDlg->SetDefaultFolder(pShellItem);
	pDlg->SetOptions(FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PICKFOLDERS); 

	//show the dialogbox and retrieve the result
	hr = pDlg->Show(m_hWnd);
	//error or canceled
	if (FAILED(hr))
	{
		pShellItem->Release();
		pDlg->Release();

		if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
			DisplayError(hr);
		return;
	}
	//we don't need the desktop shell item anymore
	pShellItem->Release();

	//we retrieve the selected item as shell item
	hr = pDlg->GetResult(&pShellItem);
	if (FAILED(hr))
	{
		pDlg->Release();

		DisplayError(hr);
		return;
	}

	//we retrieve the full file name of the selected item
	WCHAR* wsFileName;
	pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &wsFileName);

	//we don't need the selected shell item and the dialogbox anymore
	pShellItem->Release();
	pDlg->Release();

	//we store the info we need
	Send::itemType = ItemType::Folder;

	int len = StringLen(wsFileName);
	len++;

	//Send::wsParentFileName will store the selected item
	if (Send::wsParentFileName) delete[] Send::wsParentFileName;
	Send::wsParentFileName = new WCHAR[len];
	StringCopy(Send::wsParentFileName, wsFileName);
	//we don't need wsFileName anymore
	CoTaskMemFree(wsFileName);

	//Send::wsParentFileDisplayName SHOULD NOT BE DELETED: it is only a pointer witin Send::wsParentFileName
	Send::wsParentFileDisplayName = StringRevChar(Send::wsParentFileName, '\\');
	if (Send::wsParentFileDisplayName)
		Send::wsParentFileDisplayName++;

	//we display the full file name of the selected item into the editbox and enable the "Send" and "Repair" buttons
	SetWindowTextW(m_hEditItemPath, Send::wsParentFileName);
	EnableWindow(m_hButtonSend, true);
	EnableWindow(m_hCheckRepairMode, true);
}

#else

void MainDlg::PickFileXP(HWND hDlg)
{
	//we need a GetOpenFileName to retrieve a file
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hDlg;
	ofn.hInstance = Application::GetHInstance();
	ofn.lpstrFile = new WCHAR[2000];
	*ofn.lpstrFile = 0;
	ofn.nMaxFile = 2000;
	ofn.Flags = OFN_FILEMUSTEXIST;

	//display the dialogbox and retrieve the result
	if (false == GetOpenFileName(&ofn))
	{
		DWORD dwError =  CommDlgExtendedError();
		//either error or canceled
		if (dwError)
		{
			WCHAR wsError[500];
			LoadStringW(Application::GetHInstance(), dwError, wsError, 500);
			MessageBox(hDlg, wsError, 0, MB_ICONERROR);
		}
		delete[] ofn.lpstrFile;
		return;
	}

	//doing what we need with it.
	Send::itemType = ItemType::File;

	//retrieve the selected item (as path)
	int len = StringLen(ofn.lpstrFile);
	len++;

	//we store the result in Send::wsParentFileName
	if (Send::wsParentFileName) delete[] Send::wsParentFileName;
	Send::wsParentFileName = new WCHAR[len];
	StringCopy(Send::wsParentFileName, ofn.lpstrFile);
	//we don't need ofn.lpstrFile anymore
	delete[] ofn.lpstrFile;

	//wsParentFileDisplayName SHOULD NOT BE DELETED: it is a pointer within Send::wsParentFileName
	Send::wsParentFileDisplayName = StringRevChar(Send::wsParentFileName, '\\');
	Send::wsParentFileDisplayName++;

	//we set the full file name into the editbox and enable the "Send" and "Repair" buttons.
	SetWindowTextW(m_hEditItemPath, Send::wsParentFileName);
	EnableWindow(m_hButtonSend, true);
	EnableWindow(m_hCheckRepairMode, false);
}

void MainDlg::PickFolderXP(HWND hDlg)
{	
	//we need Shell Browser for this.
	BROWSEINFOW bi = {0};
	bi.hwndOwner = hDlg;
	bi.lpszTitle = L"Chose the folder you wish to send:";
	bi.ulFlags = BIF_EDITBOX;
	bi.pszDisplayName = new WCHAR[MAX_PATH];

	//the result is stored in pidlResult
	ITEMIDLIST* pidlResult;
try_again:
	pidlResult = SHBrowseForFolderW(&bi);
	//if a result
	if (pidlResult != 0)
	{
		//we retrieve the item as a shellitem so we would get its displayname
		IShellItem* pShellItem;
		HRESULT hr = SHCreateShellItem(0, 0, pidlResult, &pShellItem);
		if (FAILED(hr))
		{
			delete[] bi.pszDisplayName;
			CoTaskMemFree(pidlResult);

			DisplayError(hr);
			return;
		}

		//we retrieve the filesyspath of the selected item. failure if the selected item
		//is not part of the FILE SYSTEM!
		WCHAR* wsFullName;
		hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &wsFullName);
		if (FAILED(hr))
		{
			CoTaskMemFree(pidlResult);
			pShellItem->Release();

			MessageBox(hDlg, L"Invalid selection. Chose a directory or a drive!", L"Invalid selection!", MB_ICONWARNING);
			goto try_again;
		}

		//we don't need the shell item anymore
		pShellItem->Release();

		//ok, so the selection is ok. we fill the info
		Send::itemType = ItemType::Folder;
		int len = StringLen(wsFullName);
		len++;

		//we store the file name into Send::wsParentFileName 
		if (Send::wsParentFileName) delete[] Send::wsParentFileName;
		Send::wsParentFileName = new WCHAR[len];
		StringCopy(Send::wsParentFileName, wsFullName);
		//we don't need wsFullName anymore
		CoTaskMemFree(wsFullName);
		
		//Send::wsParentFileDisplayName SHOULD NOT BE DELETED: it is only a pointer within Send::wsParentFileName
		Send::wsParentFileDisplayName = StringRevChar(Send::wsParentFileName, '\\');
		if (Send::wsParentFileDisplayName)
			Send::wsParentFileDisplayName++;

		//we set the full file name into the editbox and enable the "Send" and "Repair" buttons.
		SetWindowTextW(m_hEditItemPath, Send::wsParentFileName);
		EnableWindow(m_hButtonSend, true);
		EnableWindow(m_hCheckRepairMode, true);

	}
	//we don't need the displayname and the pidlResult anymore
	delete[] bi.pszDisplayName;
	if (pidlResult) CoTaskMemFree(pidlResult);
}

#endif

void MainDlg::CloseAll()
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

	EnableWindow(m_hButtonSend, 0);
	SetWindowTextW(m_hEditItemPath, L"");

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
	UpdateUIDisconnected();
}

#define TYPE_INVALID	0
#define TYPE_IP			1
#define TYPE_NICKNAME	2

//the "Connect" button was pressed
void MainDlg::OnButtonConnect()
{
	_ASSERTE(Connected == Conn::NotConnected);

	//read the ip or name from the editbox.
	int len = GetWindowTextLengthW(m_hComboNickIP);
	len++;
	WCHAR* wstr = new WCHAR[len];
	GetWindowTextW(m_hComboNickIP, wstr, len);

	//checking to see whether it is an IP or a nickname
	//nick if it contains letters or ' '
	//ip if it contains only digits and '.'
	DWORD type = TYPE_INVALID;//0 - invalid; 1 - ip; 2 - nick (alpha + ' ')
	for (int i = 0; i < len - 1; i++)
		if (*(wstr + i) != '.' && (!(*(wstr + i) >= '0' && *(wstr + i) <= '9')))
		{
			if (iswalpha(*(wstr + i)) || *(wstr + i) == ' ')
				type = TYPE_NICKNAME;
			else
			{
				type = TYPE_INVALID;
				break;
			}
		}
		else type = TYPE_IP;

	if (type == TYPE_IP)
	{
		//make sure the ip is in the format: nr.nr.nr.nr and each number is less than 256

		if (*wstr == '.') {type = TYPE_INVALID; goto checkeach;}

		//the first number
		int nr = _wtoi(wstr);
		if (nr > 255) {type = TYPE_INVALID; goto checkeach;}
		
		//the second number
		WCHAR* wPos = StringCharW(wstr, '.');
		if (!wPos) {type = TYPE_INVALID; goto checkeach;}
		wPos++;
		if (!*wPos) {type = TYPE_INVALID; goto checkeach;}
		nr = _wtoi(wPos);
		if (nr > 255) {type = TYPE_INVALID; goto checkeach;}

		//the third number
		wPos = StringCharW(wPos, '.');
		if (!wPos) {type = TYPE_INVALID; goto checkeach;}
		wPos++;
		if (!*wPos) {type = TYPE_INVALID; goto checkeach;}
		nr = _wtoi(wPos);
		if (nr > 255) {type = TYPE_INVALID; goto checkeach;}

		//the fourth number
		wPos = StringCharW(wPos, '.');
		if (!wPos) {type = TYPE_INVALID; goto checkeach;}
		wPos++;
		if (!*wPos) {type = TYPE_INVALID; goto checkeach;}
		nr = _wtoi(wPos);
		if (nr > 255) {type = TYPE_INVALID; goto checkeach;}

		//there should be nothing here:
		wPos = StringCharW(wPos, '.');
		if (wPos) {type = TYPE_INVALID; goto checkeach;}
	}

checkeach:
	switch (type)
	{
	case TYPE_INVALID: MessageBox(m_hWnd, L"This IP/Nickname is not valid. \n\nNicknames can contain only letters and spaces.\nIPs contain only numbers and dots (e.g. 81.20.100.142)",
				L"Invalid Value", MB_ICONWARNING);
		delete[] wstr;
		return;

	case TYPE_IP:
		{
			//in this case, wstr is the IP: it has been written an IP and pressed the "Connect" button.
			NickNameDlg nickDlg(m_hWnd);
			if (IDOK == nickDlg.CreateModal())
			{
				//save the nick in the registry.
				//wsText SHOULD NOT be deleted: it is destroyed automatically by the dialog.
				const WCHAR* wsText = nickDlg.GetText();
				ComboBox_AddString(m_hComboNickIP, wsText);

				//we write into the registry the nickname and its associated IP
				DWORD cbData = (StringLenW(wstr) + 1) * 2;
				DWORD dwError = RegSetValueExW(m_hKey, wsText, 0, REG_SZ, (BYTE*)wstr, cbData);
				if (ERROR_SUCCESS != dwError)
				{
					DisplayError(dwError);
				}

				//we set into the registry this nickname as the last nickname the computer was connected to:
				cbData = (StringLenW(wsText) + 1) * 2;
				dwError = RegSetValueExW(m_hKey, 0, 0, REG_SZ, (BYTE*)wsText, cbData);
				if (ERROR_SUCCESS != dwError)
				{
					DisplayError(dwError);
				}

				//we set the nickname (instead of leaving the IP) in the combobox text:
				SetWindowText(m_hComboNickIP, wsText);

				//we replace the old Server IP with this one
				if (CSocketClient::m_sServerIP) delete[] CSocketClient::m_sServerIP;
				CSocketClient::m_sServerIP = StringWtoA(wstr);

				//we no longer need wstr
				delete[] wstr;

				//the connection to the server is done here:
				ConnectToServer();
			}
			else
			{
				delete[] wstr;
				return;
			}
		}
		break;

	case TYPE_NICKNAME:
		{
			//in this case, wstr is NICKNAME: it has been written a NICKNAME and pressed the "Connect" button.
			//seek value into the registry
			WCHAR wsIP[31];
			DWORD cbSize = 31;

			DWORD dwError = RegQueryValueExW(m_hKey, wstr, 0, 0, (BYTE*)wsIP, &cbSize);
			if (ERROR_SUCCESS != dwError)
			{
				delete[] wstr;

				if (dwError == ERROR_FILE_NOT_FOUND)
				{
					MessageBox(m_hWnd, L"This nickname does not exist!\n\nPlease select a nickname from the\
 list or write an IP to connect to.", L"Unrecognized nickname!", MB_ICONWARNING);
				}

				else
				{
					DisplayError(dwError);
				}
				return;
			}

			//we set into the registry this nickname as the last nickname the computer was connected to:
			DWORD cbData = (StringLenW(wstr) + 1) * 2;
			dwError = RegSetValueExW(m_hKey, 0, 0, REG_SZ, (BYTE*)wstr, cbData);
			if (ERROR_SUCCESS != dwError)
			{
				delete[] wstr;
				DisplayError(dwError);
			}

			//we no longer need the Nickname
			delete[] wstr;

			//we replace the old Server IP with this new IP (wsIP)
			if (CSocketClient::m_sServerIP) delete[] CSocketClient::m_sServerIP;
			CSocketClient::m_sServerIP = StringWtoA(wsIP);

			//we connect to the server here:
			ConnectToServer();
		}
		break;
	}
}

void MainDlg::ConnectToServer()
{
	_ASSERTE(Connected == Conn::NotConnected);

	//do not resend data!
	dwDataTransfer = 0;

	//first off, disable the other control
	EnableWindow(m_hButtonCreateConn, false);
	EnableWindow(m_hButtonConnect, false);
	EnableWindow(m_hComboNickIP, false);
	SetWindowTextW(m_hStatusText, L"The client is starting...");

	//creating the connection
	Recv::hConnThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Recv::ConnThreadProc, 0, 0, 0);
}

void MainDlg::CreateConnection()
{
	_ASSERTE(Connected == Conn::NotConnected);

	//do not resend data!
	dwDataTransfer = 0;

	//first off, disable the other controls:
	EnableWindow(m_hButtonCreateConn, false);
	EnableWindow(m_hButtonConnect, false);
	EnableWindow(m_hComboNickIP, false);
	SetWindowTextW(m_hStatusText, L"The server is starting...");
	SetWindowTextW(m_hComboNickIP, 0);

	//creating connection
	Send::hConnThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Send::ConnThreadProc, 0, 0, 0);
}

void MainDlg::UpdateUIDisconnected()
{
	//first off, enable the other controls:
	EnableWindow(m_hButtonCreateConn, true);
	EnableWindow(m_hButtonConnect, true);
	EnableWindow(m_hComboNickIP, true);

	//after the connection has ended
	EnableWindow(m_hButtonBrowse, false);

	//the buttons
	SetWindowTextW(m_hButtonCreateConn, L"Create Connection");
	EnableWindow(m_hButtonCreateConn, true);

	SetWindowTextW(m_hButtonConnect, L"Connect");
	EnableWindow(m_hButtonConnect, true);

	//status texts
	SetWindowTextW(m_hStatusSend, L"Not Sending");
	SetWindowTextW(m_hStatusRecv, L"Not Receiving");
	SetWindowTextW(m_hStatusCurrFileSend, L"none");
	SetWindowTextW(m_hStatusCurrFileRecv, L"none");

	//progress bars
	SendMessage(m_hBarRecv, PBM_SETPOS, 0, 0);
	SendMessage(m_hBarSend, PBM_SETPOS, 0, 0);

	//retrieve the last Nickname this computer was connected to:
	DWORD dwNickSize = 20;
	WCHAR wsNick[20];
	DWORD dwError = RegQueryValueExW(m_hKey, 0, 0, 0, (BYTE*)wsNick, &dwNickSize);
	if (0 != dwError && ERROR_FILE_NOT_FOUND != dwError)
	{
		DisplayError(dwError);
	}

	//if there is a default value set, we set it here. (otherwise, the value already set remains)
	if (dwError != ERROR_FILE_NOT_FOUND && *wsNick)
	{
		SetWindowTextW(m_hComboNickIP, wsNick);
	}

	//status changed to "Not Connected"
	Connected = Conn::NotConnected;
}