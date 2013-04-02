#ifndef APP_H
#define APP_H

#include "General.h"
#include "MainDlg.h"
#include "NetworkManager.h"
#include "Events.h"

class App
{
private:
	//the icon of the application
	HICON		m_hIcon;
	//the hkey HKLM\Software\FeoggouApp\SearchApp
	HKEY		m_hKey;
	//the instance of the application
	HINSTANCE	m_hInst;
	//specifies whether the window is currently minimized or not.
	BOOL		m_bIsMinimized;
	//the main dialog: FileTransferAppDlg
	MainDlg		m_mainDlg;
	NetworkManager	m_networkManager;

private:
	//it's a singletone: the App is retrieved from a static member function
	App(void);
	~App(void);

public:
	//retrieves the main dialog
	MainDlg& GetMainDlg() {return m_mainDlg;}
	//retrieves the network manager
	NetworkManager& GetNetworkManager() {return m_networkManager;}
	//creates (and displays) the main dialog
	void CreateMainDlg() {m_mainDlg.DoModal(0);}
	//sets the HINSTANCE of the application to smth
	void SetInstance(HINSTANCE hInst) {m_hInst = hInst;}
	//retrieves the HINSTANCE of the application
	HINSTANCE GetInstance() {return m_hInst;}
	//returns whether the main dialog is minimized or not
	bool IsMinimized() const {return m_bIsMinimized;}
	//retrieves/creates the App object
	inline static App& Get() {static App myApp;	return myApp;}
	//called by OnButtonConnect. it connects the application to the server (the other computers).
	void ConnectToServer();
	//removes the tray icon from the system tray
	void DestroyTrayIcon();
	//closes the sockets, files, deletes buffers, etc.
	void CloseAll();
	//gets the HWND of the m_mainDlg dialog.
	HWND GetHWND() {return m_mainDlg.GetHWND();}

	//PRIVATE FUNCTIONS:
private:
	//called by the MainDlg at OnInitDialog to initialize the application.
	void Initialize();
	//called by NetworkManager only.
	void OnNetworkEvent(const Events::NetworkEvent& anEvent);

	//template<typename ClassName>
	friend class Dialog;
	friend class NetworkManager;
	friend MainDlg;
};

#endif//APP_H