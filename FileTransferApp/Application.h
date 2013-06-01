#pragma once

#include "General.h"

#include <memory>
#include <vector>

class MainDlg;

class Application
{
public:
	Application(HINSTANCE hInst);
	~Application(void);

	void ShowMainDialog();

	HWND GetMainWindow();
	HICON GetAppIcon() {return m_hIcon;}

	const std::wstring& GetCurrentFriend() const {return m_sCurrentFriend;}
	const std::vector<std::wstring>& GetFriends() const {return m_friends;}
	void SaveFriend(const std::wstring& name, const std::wstring& ip);
	const std::wstring&& GetIpOfFriend(const std::wstring& friend_name);
	void SetLastFriend(const std::wstring& last_friend_name);
	const std::wstring&& GetLastFriend() const;

public:
	static HINSTANCE GetHInstance() {_ASSERT(s_pInst); return s_hInst;}
	static Application* GetInst() {_ASSERT(s_pInst); return s_pInst;}

private:
	void LoadFriends();

private:
	static HINSTANCE	s_hInst;
	static Application*	s_pInst;

	HICON	m_hIcon;
	//HKLM\Software\FeoggouApp\FileTransferer
	HKEY		m_hKey;
	std::unique_ptr<MainDlg>	m_pMainDlg;

	std::vector<std::wstring>		m_friends;
	std::wstring					m_sCurrentFriend;
};

#define theApp Application::GetInst()