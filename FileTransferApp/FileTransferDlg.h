#pragma once
#include "General.h"

#ifndef FILETRANSFERDLG_H
#define FILETRANSFERDLG_H

class CFileTransferDlg
{
	//PRIVATE DATA
private:
	//the HWND of the dialog
	HWND		m_hDlg;
	//the icon of the application
	HICON		m_hIcon;
	//the hkey HKLM\Software\FeoggouApp\SearchApp
	HKEY		m_hKey;

	//PUBLIC DATA
public:
	//STATIC ITEMS
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

	//specifies whether the windows is currently minimized or not.
	static BOOL		m_bIsMinimized;

public:
	CFileTransferDlg(void)
	{
		m_hDlg = NULL;
		m_hKey = NULL;
	}

	~CFileTransferDlg(void)
	{
	}

	void DoModal();

	//PRIVATE FUNCTIONS
private:
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//MESSAGE HANDLERS
	void OnInitDialog();
	//called when the user presses the "Connect" button
	void OnButtonConnect();
	//called when the user presses the "Create Connection" button
	void CreateConnection();
	//called when the user presses the "Browse" button
	void OnBrowse();

	//OTHER FUNCTIONS
	//called by OnButtonConnect. it connects the application to the server (the other computers).
	void ConnectToServer();
	//removes the tray icon from the system tray
	void DestroyTrayIcon();
	//closes the sockets, files, deletes buffers, etc.
	void CloseAll();
	//updates the UI after the connection has been closed
	void UpdateUIDisconnected();

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
};

#endif//FILETRANSFERDLG_H