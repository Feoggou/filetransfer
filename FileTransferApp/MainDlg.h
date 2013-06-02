#pragma once
#include "General.h"
#include "resource.h"

#include "Dialog.h"

#include <string>

class MainDlg : public Dialog
{
	enum StringType {StringType_Invalid, StringType_NickName, StringType_Ip};

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
	MainDlg() : Dialog(nullptr, IDD_FILETRANSFERAPP_DIALOG) {}

	~MainDlg(void) {}

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
	//called by OnClose and RestoreFromTray
	void DestroyTrayIcon();
	//closes the sockets, files, deletes buffers, etc.
	void CloseAll();
	//updates the UI after the connection has been closed
	//called by CloseAll
	void UpdateUIDisconnected();

	void AppendAboutSysMenuItem();
	void SetWindowIcon();
	void InitializeProgressBars();
	void InitializeLabels();
	void InitializeButtons();
	void ConnectToNickname(const std::wstring& name);
	void ConnectToIp(const std::wstring& ip);

	void IsNicknameOrIp(const std::wstring& s, StringType& type) const;

	//called by OnBrowse
	static void PickFile(HWND hDlg);
	//called by OnBrowse
	static void PickFolder(HWND hDlg);

private:
	void OnClose() override;
	void OnSysCommand(WPARAM cmd, int x, int y) override;
	void OnCommand(WORD source, WORD id, HWND hControl) override;
	INT_PTR OnDialogProcedure(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void OnSize(WPARAM type, int new_width, int new_height) override;

private:
	void OnAbout();
	void OnButtonSend();
	void RestoreFromTray();

	void OnEnableChild(HWND hChild, bool enable);
	void OnSetItemText(LPARAM lParam, LPCWSTR text);

	void OnTrayMessage(LPARAM lParam);
	void OnMinimized();
	//TODO: split in two, based on path = NULL or not
	void OnShowMessageBox(LPCWSTR message, LPCWSTR path);
};