#include "MainDlg.h"
#include "resource.h"
#include "AboutDlg.h"

#include "Application.h"
#include "Tools.h"

#include "Recv.h"
#include "Send.h"

#include "NickNameDlg.h"
#include "FilePicker.h"
#include "FolderPicker.h"
#include "String.h"

#include <algorithm>

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
	nid.hIcon = theApp->GetAppIcon();
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

void MainDlg::AppendAboutSysMenuItem()
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

void MainDlg::SetWindowIcon()
{
	//setting the icon
	HICON hIcon = theApp->GetAppIcon();
	SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
}

void MainDlg::InitializeProgressBars()
{
	//initializing handles and controls: here the progress bars
	m_hBarRecv = GetDlgItem(m_hWnd, IDC_PROG_RECV);
	m_hBarSend = GetDlgItem(m_hWnd, IDC_PROG_SEND);

	//set the range of the progress bars
	SendMessageW(m_hBarRecv, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessageW(m_hBarSend, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
}

void MainDlg::InitializeLabels()
{
	m_hStatusText = GetDlgItem(m_hWnd, IDC_STATUS);
	m_hStatusSend = GetDlgItem(m_hWnd, IDC_ST_SEND);
	m_hStatusRecv = GetDlgItem(m_hWnd, IDC_ST_RECV);
	m_hStatusCurrFileSend = GetDlgItem(m_hWnd, IDC_ST_CURFILE_S);
	m_hStatusCurrFileRecv = GetDlgItem(m_hWnd, IDC_ST_CURFILE_R);
}

void MainDlg::InitializeButtons()
{
	m_hButtonConnect = GetDlgItem(m_hWnd, IDC_BUTTON_CONNECT);
	m_hButtonCreateConn = GetDlgItem(m_hWnd, IDC_BUTTON_CREATE_CONNECTION);
	m_hButtonSend = GetDlgItem(m_hWnd, IDC_BUTTON_SEND);
	m_hButtonBrowse = GetDlgItem(m_hWnd, IDC_BUTTON_BROWSE);
}

void MainDlg::OnInitDialog()
{
	AppendAboutSysMenuItem();
	SetWindowIcon();

	InitializeProgressBars();
	InitializeLabels();
	InitializeButtons();

	//initializing handles and controls
	m_hCheckRepairMode = GetDlgItem(m_hWnd, IDC_CHK_REPAIRMODE);
	m_hComboNickIP = GetDlgItem(m_hWnd, IDC_COMBO_IP);
	m_hEditItemPath = GetDlgItem(m_hWnd, IDC_EDIT_BROWSE);

	std::wstring current_friend = theApp->GetCurrentFriend();
	SetWindowTextW(m_hComboNickIP, current_friend.data());

	std::vector<std::wstring> friends = theApp->GetFriends();
	for (std::vector<std::wstring>::const_iterator i = std::begin(friends); i != std::end(friends); ++i) {
		SendMessage(m_hComboNickIP, CB_ADDSTRING, 0, (LPARAM)i->data());
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
		//No longer using files directly from MainDlg!
		/*if (Send::File.IsOpened())
				Send::File.Close();*/
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

void MainDlg::PickFile(HWND hDlg)
{
	std::wstring result = FilePicker(hDlg)();

	if (!result.empty()) {
		////doing what we need with it.
		//Send::itemType = ItemType_File;

		////retrieve the selected item (as path)
		//int len = result.length();
		//len++;

		////we store the result in Send::wsParentFileName
		//if (Send::wsParentFileName) delete[] Send::wsParentFileName;
		//Send::wsParentFileName = new WCHAR[len];
		//StringCopy(Send::wsParentFileName, result.data());

		////wsParentFileDisplayName SHOULD NOT BE DELETED: it is a pointer within Send::wsParentFileName
		//Send::wsParentFileDisplayName = StringRevChar(Send::wsParentFileName, '\\');
		//Send::wsParentFileDisplayName++;

		//we set the full file name into the editbox and enable the "Send" and "Repair" buttons.
		SetWindowTextW(m_hEditItemPath, result.data());
		EnableWindow(m_hButtonSend, true);
		EnableWindow(m_hCheckRepairMode, false);
	}
}

void MainDlg::PickFolder(HWND hDlg)
{	
	std::wstring result = FolderPicker(hDlg)();

	if (!result.empty()) {
		////ok, so the selection is ok. we fill the info
		//Send::itemType = ItemType_Folder;
		//int len = result.length();
		//len++;

		////we store the file name into Send::wsParentFileName 
		//if (Send::wsParentFileName) delete[] Send::wsParentFileName;
		//Send::wsParentFileName = new WCHAR[len];
		//StringCopy(Send::wsParentFileName, result.data());
		//
		////Send::wsParentFileDisplayName SHOULD NOT BE DELETED: it is only a pointer within Send::wsParentFileName
		//Send::wsParentFileDisplayName = StringRevChar(Send::wsParentFileName, '\\');
		//if (Send::wsParentFileDisplayName)
		//	Send::wsParentFileDisplayName++;

		//we set the full file name into the editbox and enable the "Send" and "Repair" buttons.
		SetWindowTextW(m_hEditItemPath, result.data());
		EnableWindow(m_hButtonSend, true);
		EnableWindow(m_hCheckRepairMode, true);
	}
}

void MainDlg::CloseAll()
{
	//we need to close connection
	bOrderEnd = TRUE;
	//no data transfer is allowed anymore (this will deny Send::Thread and Recv::Thread from starting to transfer)
	dwDataTransfer = 0;

	//TODO:
	//m_receiveWorker.Cleanup();
	//m_sendWorker.Cleanup();
	//TODO: should I use different classes, or ReceiveFilesThread and SendFilesThread?

	////cleaning up
	////receive
	//m_recv.CloseFile();
	//if (Recv::wsParentDisplayName) {delete[] Recv::wsParentDisplayName; Recv::wsParentDisplayName = 0;}
	//if (Recv::wsChildFileName) {delete[] Recv::wsChildFileName; Recv::wsChildFileName = NULL;}

	////send
	//if (Send::File.IsOpened()) Send::File.Close();
	//if (Send::wsParentFileName) {delete[] Send::wsParentFileName; Send::wsParentFileName = NULL;}

	//EnableWindow(m_hButtonSend, 0);
	//SetWindowTextW(m_hEditItemPath, L"");

	////close the sockets BEFORE closing the threads!
	//int nError;
	////if the socket was not already closed, we close it.
	////we delete the pointer after the threads have closed only.
	//m_send.CloseSocket();
	//m_recv.CloseSocket();

	////these 2 must have been closed allready
	//m_recv.StopThreads();
	//m_send.StopThreads();

	//update the UI: it is possible that we do not close the program now.
	UpdateUIDisconnected();
}

void MainDlg::IsNicknameOrIp(const std::wstring& s, StringType& type) const
{
	//checking to see whether it is an IP or a nickname
	//nick if it contains letters or ' '
	//ip if it contains only digits and '.'
	type = StringType_Invalid;//0 - invalid; 1 - ip; 2 - nick (alpha + ' ')

	size_t len = s.length();
	for (int i = 0; i < len - 1; i++)
		if (s[i] != '.' && (!(s[i] >= '0' && s[i] <= '9')))
		{
			if (iswalpha(s[i]) || s[i] == ' ')
				type = StringType_NickName;
			else
			{
				type = StringType_Invalid;
				break;
			}
		}
		else type = StringType_Ip;

	if (type == StringType_Ip)
	{
		//make sure the ip is in the format: nr.nr.nr.nr and each number is less than 256

		if (s[0] == '.') {type = StringType_Invalid; return;}

		//the first number
		int nr = _wtoi(s.data());
		if (nr > 255) {type = StringType_Invalid; return;}

		DWORD byte0, byte1, byte2, byte3;
		int count_successful = swscanf(s.data(), L"%d.%d.%d.%d", &byte0, &byte1, &byte2, &byte3);
		//_ASSERTE(count_successful == 4);

		if (count_successful != 4) {
			type = StringType_Invalid;
			return;
		}

		if (byte0 > 255 || byte1 > 255 || byte2 > 255 || byte3 > 255) {
			type = StringType_Invalid;
			return;
		}
	}
}

void MainDlg::ConnectToNickname(const std::wstring& name)
{
	//in this case, wstr is NICKNAME: it has been written a NICKNAME and pressed the "Connect" button.
	//seek value into the registry

	std::wstring ip = theApp->GetIpOfFriend(name);

	theApp->SetLastFriend(name);

	//we replace the old Server IP with this new IP (wsIP)
	if (SocketClient::m_sServerIP) delete[] SocketClient::m_sServerIP;
	SocketClient::m_sServerIP = StringWtoA(ip.data());

	//we connect to the server here:
	ConnectToServer();
}

void MainDlg::ConnectToIp(const std::wstring& ip)
{
	//in this case, wstr is the IP: it has been written an IP and pressed the "Connect" button.
	NickNameDlg nickDlg(m_hWnd);
	if (IDOK == nickDlg.CreateModal())
	{
		//save the nick in the registry.
		//wsText SHOULD NOT be deleted: it is destroyed automatically by the dialog.
		const WCHAR* wsText = nickDlg.GetText();
		ComboBox_AddString(m_hComboNickIP, wsText);

		theApp->SaveFriend(wsText, ip);

		//we set the nickname (instead of leaving the IP) in the combobox text:
		SetWindowText(m_hComboNickIP, wsText);

		//we replace the old Server IP with this one
		if (SocketClient::m_sServerIP) delete[] SocketClient::m_sServerIP;
		SocketClient::m_sServerIP = StringWtoA(ip.data());

		//the connection to the server is done here:
		ConnectToServer();
	}
}

//the "Connect" button was pressed
void MainDlg::OnButtonConnect()
{
	_ASSERTE(Connected == Conn::NotConnected);

	//read the ip or name from the editbox.
	int len = GetWindowTextLengthW(m_hComboNickIP);
	len++;
	WCHAR* wstr = new WCHAR[len];
	GetWindowTextW(m_hComboNickIP, wstr, len);

	StringType type;
	IsNicknameOrIp(wstr, type);

	if  (type == StringType_NickName) {
		ConnectToNickname(wstr);
	} else if (type == StringType_Ip) {
		ConnectToIp(wstr);
	} else {
		MessageBox(m_hWnd, L"This IP/Nickname is not valid. \n\nNicknames can contain only letters and spaces.\nIPs contain only numbers and dots (e.g. 81.20.100.142)",
				L"Invalid Value", MB_ICONWARNING);
		delete[] wstr;
		return;
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

	//TODO: choose other method
	////creating the connection
	//m_recv.StartConnThread(/*server*/ false);
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

	//TODO: choose other method
	////creating connection
	//m_send.StartConnThread(/*server*/ true);
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

	std::wstring last_friend = theApp->GetLastFriend();
	if (!last_friend.empty()) {
		SetWindowTextW(m_hComboNickIP, last_friend.data());
	}

	//status changed to "Not Connected"
	Connected = Conn::NotConnected;
}

////TODO: remove socket retrieval later. Now we need them, because they're used everywhere.
//Socket* MainDlg::GetReceiveSocket()
//{
//	return m_recv.GetSocket();
//}
//
//Socket* MainDlg::GetSendSocket()
//{
//	return m_send.GetSocket();
//}