#pragma once
#include "General.h"
#include "Dialog.h"
#include "resource.h"

#ifndef MAINDLG_H
#define MAINDLG_H

class App;

class MainDlg: public Dialog
{
	//PUBLIC DATA
public:
	enum UIEvent {
		//the other computer has closed the connection (e.g. closed the application)
		OtherComputerClosedConn,
		//prepare the UI for a receiving operation (of a file or folder)
		InitReceiveItems,
		//prepare the UI for a receiving operation (of a file or folder)
		InitSendItems
	};

	//STATIC ITEMS
	//static, because they are used in non-member functions
	//the checkbox that allows the user to fix a folder (when receiving)
	static HWND		m_hCheckRepairMode;
	//the status that is displayed regarding the connection
	static HWND		m_hStatusText;
	//the "Connect" button
	static HWND		m_hButtonConnect;
	//the "Create Connection" button
	static HWND		m_hButtonCreateConn;
	//the "Send" button
	static HWND		m_hButtonSend;
	//the "Browse" button
	static HWND		m_hButtonBrowse;
	//the editbox where the file/folder path is displayed
	static HWND		m_hEditItemPath;
	//the combobox that contains Nicknames: the editbox of the combo can contain either a nick or an IP
	static HWND		m_hComboNickIP;

	//the status of the item(s) that are being sent
	static HWND		m_hStatusSend;
	//the status of the item(s) that are being received
	static HWND		m_hStatusRecv;
	//the full name of the file that is being sent
	static HWND		m_hStatusCurrFileSend;
	//the full name of the file that is being received
	static HWND		m_hStatusCurrFileRecv;
	//the progressbar that shows the progress of the send operation
	static HWND		m_hBarSend;
	//the progressbar that shows the progress of the receive operation
	static HWND		m_hBarRecv;

private:
	MainDlg(void):Dialog(IDD_FILETRANSFERAPP_DIALOG) {}
	//updates the UI after the connection has been closed
	void UpdateUIDisconnected();
	//void UpdateUI(UIEvent uiEvent);

	//PRIVATE FUNCTIONS
protected:
	virtual INT_PTR DoDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

	//MESSAGE HANDLERS
	void OnInitDialog();
	//called when the user presses the "Connect" button
	void OnButtonConnect();
	//called when the user presses the "Create Connection" button
	void OnCreateConnection();
	//called when the user presses the "Browse" button
	void OnBrowse();
	void ConnectToServer();

	//OTHER FUNCTIONS

#if _WIN32_WINNT == _WIN32_WINNT_VISTA
#define PickFile	PickFileVista
#define PickFolder	PickFolderVista

	//displays a Vista-compatible dialogbox that allows the user to select a file to send to the other computer
	static void PickFileVista();
	//displays a Vista-compatible dialogbox that allows the user to select a folder to send to the other computer
	static void PickFolderVista();

#else

#define PickFile	PickFileXP
#define PickFolder	PickFolderXP

	//displays an XP-compatible dialogbox that allows the user to select a file to send to the other computer
	static void PickFileXP();
	//displays an XP-compatible dialogbox that allows the user to select a file to send to the other computer
	static void PickFolderXP();
#endif

	void OnClose();
	void OnAbout();
	void OnButtonSend();
	void OnButtonExit();
	//restore the program from "minimized to tray" state.
	//the item menu is "Show"
	void OnRestoreProgram(WORD wFromWhere);
	//the user has chosen the "Exit" item from the system tray menu.
	void OnExitProgram(WORD wFromWhere);
	void OnSize(int nType, WORD NewWidth, WORD NewHeight);

	friend class App;

public:
	void SendEvent(const Events::MainDlgEvent& anEvent);
};

#endif//MAINDLG_H