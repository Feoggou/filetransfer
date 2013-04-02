#include "MainDlg.h"
#include "resource.h"
#include "AboutDlg.h"

#include "Recv.h"
#include "Send.h"

#include "NickNameDlg.h"

#include "SocketClient.h"
#include "Exceptions.h"
#include "App.h"

#define IDMI_ABOUT			0x0010

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


INT_PTR MainDlg::DoDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	{
	case WM_SYSCOMMAND:
		{
			if ((wParam & 0xFFF0) == IDMI_ABOUT)
			{
				CAboutDlg dlgAbout;
				dlgAbout.DoModal(m_hDlg);
			}
		}
		break;

	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDC_ABOUT:
				{
					OnAbout();
				}
				break;

			case IDC_BUTTON_SEND:
				{
					OnButtonSend();
				}
				break;

			case IDC_BUTTON_CONNECT:
				{
					OnButtonConnect();
				}
				break;

			case IDC_BUTTON_CREATE_CONNECTION:
				{
					OnCreateConnection();
				}
				break;

			case IDC_BUTTON_BROWSE:
				{
					OnBrowse();
				}
				break;

			case IDC_EXIT:
				{
					OnButtonExit();
				}
				break;

				//restore the program from "minimized to tray" state.
				//the item menu is "Show"
			case ID_RESTOREPROG:
				{
					OnRestoreProgram(HIWORD(wParam));
				}
				break;

				//the user has chosen the "Exit" item from the system tray menu.
				//we execute the same "Close" command
			case ID_EXITPROG:
				{
					OnExitProgram(HIWORD(wParam));
				}
				break;
			}
		}
		break;

		//WPARAM - HWND of the child
		//LPARAM - 1: enable; 0: disable
	case WM_ENABLECHILD:
		{
			EnableWindow((HWND)wParam, (BOOL)lParam);
		}
		break;

		//LPARAM: 0 - for send info; 1 - for send curfile; 
		//        2 - for recv info; 3 - for recv curfile;
		//WPARAM: text
	case WM_SETITEMTEXT:
		{
			switch (lParam)
			{
			case 0: SetWindowText(m_hStatusSend, (LPCWSTR)wParam); break;
			case 1: SetWindowText(m_hStatusCurrFileSend, (LPCWSTR)wParam); break;
			case 2: SetWindowText(m_hStatusRecv, (LPCWSTR)wParam); break;
			case 3: SetWindowText(m_hStatusCurrFileRecv, (LPCWSTR)wParam); break;
			}
		}
		break;

		//closes the sockets, the threads, updates the UI after disconnection, deletes buffers
		//resets variables
	case WM_CLOSECONNECTION:
		{
			theApp.CloseAll();
		}
		break;

		//when the user has right-clicked on the system tray icon of the program
		//create and display the menu
	case WM_TRAYMSG:
		if (lParam == WM_RBUTTONUP)
		{	
			HMENU hMenu = CreatePopupMenu();
			AppendMenuW(hMenu, MF_STRING, ID_RESTOREPROG, L"&Show");
			AppendMenuW(hMenu, MF_SEPARATOR, 0, 0);
			AppendMenuW(hMenu, MF_STRING, ID_EXITPROG, L"E&xit");
			
			POINT point;
			GetCursorPos(&point);
			TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, 0, m_hDlg, 0);
		}
		break;

		//when the user has chosen to minimize the dialogbox, we actually hide it
		//and display an icon in the system tray.
	case WM_SIZE:
		{
			OnSize(wParam, LOWORD(lParam), HIWORD(wParam));
		}
		break;

	case WM_SHOWMESSAGEBOX:
		{
			//if it is the receiver:
			if (wParam != 0)
			{
				WCHAR* wsMessage = (WCHAR*)lParam;
				WCHAR* wsPath = (WCHAR*)wParam;
				if (MessageBoxW(m_hDlg, wsMessage, L"Transfer finished", MB_YESNO) == IDYES)
				{
					ShellExecute(0, L"explore", wsPath, NULL, NULL, SW_SHOWNORMAL);
				}

				delete[] wsMessage;
				delete[] wsPath;

				//we finally reset the status text and progressbar.
				SendMessage(m_hDlg, WM_SETITEMTEXT, (WPARAM)L"Not Receiving", 2);
				SendMessage(m_hDlg, WM_SETITEMTEXT, (WPARAM)L"none", 3);
				SendMessage(MainDlg::m_hBarRecv, PBM_SETPOS, 0, 0);
			}

			//if it is the sender:
			else
			{
				MessageBoxW(m_hDlg, (WCHAR*)lParam, L"Transfer finished!", MB_ICONINFORMATION);

				//we reset the progressbar only after
				SendMessage(m_hDlg, WM_SETITEMTEXT, (WPARAM)L"Not Sending", 0);
				SendMessage(m_hDlg, WM_SETITEMTEXT, (WPARAM)L"none", 1);
				SendMessage(MainDlg::m_hBarSend, PBM_SETPOS, 0, 0);
				SendMessage(MainDlg::m_hCheckRepairMode, BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessage(m_hDlg, WM_ENABLECHILD, (WPARAM)MainDlg::m_hCheckRepairMode, 1);
			}
		}
		break;
	}

	return Dialog::DoDlgProc(uMsg, wParam, lParam);
}

#include <math.h>

void MainDlg::OnInitDialog()
{
	//appending the "About" item in the system menu
	{
		HMENU hMenu = GetSystemMenu(m_hDlg, FALSE);
		int nID = GetMenuItemCount(hMenu);
		MENUITEMINFO mii = {0};
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID;
		mii.fType = MFT_STRING;
		mii.wID = IDMI_ABOUT;
		mii.dwTypeData = L"&About";

		InsertMenuItemW(hMenu, nID - 2, TRUE, &mii);
	}

	//initializing handles and controls: here the progress bars
	m_hBarRecv = GetDlgItem(m_hDlg, IDC_PROG_RECV);
	m_hBarSend = GetDlgItem(m_hDlg, IDC_PROG_SEND);

	//set the range of the progress bars
	SendMessageW(m_hBarRecv, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessageW(m_hBarSend, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

	//initializing handles and controls
	m_hCheckRepairMode = GetDlgItem(m_hDlg, IDC_CHK_REPAIRMODE);
	m_hStatusText = GetDlgItem(m_hDlg, IDC_STATUS);
	m_hButtonConnect = GetDlgItem(m_hDlg, IDC_BUTTON_CONNECT);
	m_hButtonCreateConn = GetDlgItem(m_hDlg, IDC_BUTTON_CREATE_CONNECTION);
	m_hComboNickIP = GetDlgItem(m_hDlg, IDC_COMBO_IP);
	m_hButtonSend = GetDlgItem(m_hDlg, IDC_BUTTON_SEND);
	m_hButtonBrowse = GetDlgItem(m_hDlg, IDC_BUTTON_BROWSE);
	m_hEditItemPath = GetDlgItem(m_hDlg, IDC_EDIT_BROWSE);

	m_hStatusSend = GetDlgItem(m_hDlg, IDC_ST_SEND);
	m_hStatusRecv = GetDlgItem(m_hDlg, IDC_ST_RECV);
	m_hStatusCurrFileSend = GetDlgItem(m_hDlg, IDC_ST_CURFILE_S);
	m_hStatusCurrFileRecv = GetDlgItem(m_hDlg, IDC_ST_CURFILE_R);

	theApp.Initialize();
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
	int result = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTBUTTON | TPM_NOANIMATION, rect.left, rect.bottom, 0, m_hDlg, 0);

	if (result)
	{
		if (Send::File.IsOpened())
				Send::File.Close();
	}

	switch (result)
	{
	case 1://FILE
		{
			PickFile();
		}
		break;

	case 2://FOLDER
		{
			PickFolder();
		}
		break;
	}
}

#if _WIN32_WINNT == 0x0600
void MainDlg::PickFileVista()
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
	hr = pDlg->Show(hMainWnd);
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

void MainDlg::PickFolderVista()
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
	hr = pDlg->Show(hMainWnd);
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

void MainDlg::PickFileXP()
{
	//we need a GetOpenFileName to retrieve a file
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = theApp.GetHWND();
	ofn.hInstance = theApp.m_hInst;
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
			LoadStringW(theApp.m_hInst, dwError, wsError, 500);
			MessageBox(theApp.GetHWND(), wsError, 0, MB_ICONERROR);
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

void MainDlg::PickFolderXP()
{	
	//we need Shell Browser for this.
	BROWSEINFOW bi = {0};
	bi.hwndOwner = theApp.GetHWND();
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

			MessageBox(theApp.GetHWND(), L"Invalid selection. Chose a directory or a drive!", L"Invalid selection!", MB_ICONWARNING);
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
	case TYPE_INVALID: MessageBox(m_hDlg, L"This IP/Nickname is not valid. \n\nNicknames can contain only letters and spaces.\nIPs contain only numbers and dots (e.g. 81.20.100.142)",
				L"Invalid Value", MB_ICONWARNING);
		delete[] wstr;
		return;

	case TYPE_IP:
		{
			//in this case, wstr is the IP: it has been written an IP and pressed the "Connect" button.
			NickNameDlg nickDlg;
			if (IDOK == nickDlg.DoModal(m_hDlg))
			{
				//save the nick in the registry.
				//wsText SHOULD NOT be deleted: it is destroyed automatically by the dialog.
				const WCHAR* wsText = nickDlg.GetText();
				ComboBox_AddString(m_hComboNickIP, wsText);

				//we write into the registry the nickname and its associated IP
				DWORD cbData = (StringLenW(wstr) + 1) * 2;
				try
				{
					WNE(RegSetValueExW(theApp.m_hKey, wsText, 0, REG_SZ, (BYTE*)wstr, cbData));

					//we set into the registry this nickname as the last nickname the computer was connected to:
					cbData = (StringLenW(wsText) + 1) * 2;
					WNE(RegSetValueExW(theApp.m_hKey, 0, 0, REG_SZ, (BYTE*)wsText, cbData));

					//we set the nickname (instead of leaving the IP) in the combobox text:
					SetWindowText(m_hComboNickIP, wsText);

					//we replace the old Server IP with this one
					if (SocketClient::m_sServerIP) delete[] SocketClient::m_sServerIP;
					SocketClient::m_sServerIP = StringWtoA(wstr);

					delete[] wstr;
					wstr = NULL;

					//the connection to the server is done here:
					ConnectToServer();
				}

				catch (Exceptions::WindowException& e)
				{
					//we no longer need wstr
					if (wstr)
						delete[] wstr;
					Exceptions::ReportError(e.what(), "The written IP could not be saved");
				}
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

			try
			{
				DWORD dwError = RegQueryValueExW(theApp.m_hKey, wstr, 0, 0, (BYTE*)wsIP, &cbSize);
				if (ERROR_SUCCESS != dwError)
				{
					delete[] wstr;

					if (dwError == ERROR_FILE_NOT_FOUND)
					{
						MessageBox(m_hDlg, L"This nickname does not exist!\n\nPlease select a nickname from the\
	 list or write an IP to connect to.", L"Unrecognized nickname!", MB_ICONWARNING);
					}

					else
					{
						ThrowWin(dwError);
					}
					return;
				}

				//we set into the registry this nickname as the last nickname the computer was connected to:
				DWORD cbData = (StringLenW(wstr) + 1) * 2;
				WNE(RegSetValueExW(theApp.m_hKey, 0, 0, REG_SZ, (BYTE*)wstr, cbData));
			}

			catch (Exceptions::WindowException& e)
			{
				Exceptions::ReportError(e.what(), "The IP corresponding to the nickname given could not be retrieved");
			}

			//we no longer need the Nickname
			if (wstr) delete[] wstr;

			//we replace the old Server IP with this new IP (wsIP)
			if (SocketClient::m_sServerIP) delete[] SocketClient::m_sServerIP;
			SocketClient::m_sServerIP = StringWtoA(wsIP);

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

void MainDlg::OnCreateConnection()
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
	DWORD dwError = RegQueryValueExW(theApp.m_hKey, 0, 0, 0, (BYTE*)wsNick, &dwNickSize);
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

void MainDlg::OnClose()
{
	if (dwDataTransfer)
	{
		if (MessageBox(m_hDlg, L"File transfer is not finished. Are you sure you want to cancel the transfer?",
	L"File Transfer cancelling", MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
			return;
	}

	theApp.DestroyTrayIcon();
	theApp.CloseAll();

	//EndDialog(m_hDlg, IDCANCEL);
	Dialog::OnCancel();
}

void MainDlg::OnAbout()
{
	try
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal(m_hDlg);
	}

	catch (Exceptions::WindowException& e)
	{
		Exceptions::ReportError(e.what(), "The \"About\" box could not be created.");
	}
}

void MainDlg::OnButtonSend()
{
	//get ready the filename
	SetDlgItemTextW(m_hDlg, IDC_ST_SEND, L"Not Sending");

	//use the thread for sending the data
	dwDataTransfer |= DATATRANSF_SEND;
	
	//while sending, we disable the "Send" and the "Browse" buttons
	EnableWindow(m_hButtonSend, false);
	EnableWindow(m_hButtonBrowse, false);
}

void MainDlg::OnButtonExit()
{
	//when we press the "Exit" button, we close the dialog as we do if
	//the user pressed the X button of the dialogbox.
	SendMessage(m_hDlg, WM_CLOSE, 0, 0);
}

void MainDlg::OnRestoreProgram(WORD wFromWhere)
{
	if (wFromWhere == 0)
	{
		theApp.DestroyTrayIcon();
		ShowWindow(m_hDlg, 1);
		theApp.m_bIsMinimized = FALSE;
	}
}

void MainDlg::OnExitProgram(WORD wFromWhere)
{
	if (wFromWhere == 0)
	{
		SendMessage(m_hDlg, WM_CLOSE, 0, 0);
	}
}

void MainDlg::OnSize(int nType, WORD NewWidth, WORD NewHeight)
{
	if (nType == SIZE_MINIMIZED)
	{
		_ASSERT(!theApp.m_bIsMinimized);
		theApp.m_bIsMinimized = TRUE;

		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = m_hDlg;
		nid.uID = SYSTRAY_ICON;
		nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nid.uCallbackMessage = WM_TRAYMSG;
		nid.hIcon = theApp.m_hIcon;
		StringCopyW(nid.szTip, L"File Transfer Application");

		Shell_NotifyIcon(NIM_ADD, &nid);
		ShowWindow(m_hDlg, 0);
	}
}

void MainDlg::SendEvent(const Events::MainDlgEvent& anEvent)
{
	Events::MainDlgEvent::UIEvent uiEvent = anEvent.WhatHappened();

	switch (uiEvent)
	{
	case UIEvent::OtherComputerClosedConn:
		{
			SendMessage(MainDlg::m_hStatusText, WM_SETTEXT, 0, (LPARAM)L"The other computer has closed the connection!");
			PostMessage(hMainWnd, WM_CLOSECONNECTION, 0, 0);
		}
		break;

	case UIEvent::InitReceiveItems:
		{
			PostMessage(MainDlg::m_hBarRecv, PBM_SETPOS, 0, 0);
		}
		break;
	}
}