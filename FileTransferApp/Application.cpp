#include "Application.h"
#include "Exception.h"
#include "MainDlg.h"

#include "CRC.h"

Application* Application::s_pInst = nullptr;
HINSTANCE Application::s_hInst = NULL;

Application::Application(HINSTANCE hInst)
	: m_hIcon(NULL),
	m_hKey(NULL)
{
	s_hInst = hInst;

	_ASSERT(!s_pInst);

	s_pInst = this;

	CoInitialize(0);

	//initialize the common controls
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	//initialize the sockets
	int nError = CSamSocket::InitSockets();
	if (nError)
	{
		THROW(WindowsException, nError);

		DisplayError(nError);
		//return -1;
	}

	crcInit();

	m_hIcon = LoadIcon(s_hInst, MAKEINTRESOURCE(IDR_MAINFRAME));
}


Application::~Application(void)
{
	CloseHandle(m_hKey);

	//uninitialize the sockets
	int nError = CSamSocket::UninitSockets();
	if (nError)
	{
		DisplayError(nError);
	}

	CoUninitialize();
}

void Application::ShowMainDialog()
{
	_ASSERT(!m_pMainDlg.get());

	m_pMainDlg.reset(new MainDlg);
	m_pMainDlg->CreateModal();
}

HWND Application::GetMainWindow()
{
	return m_pMainDlg->GetHandle();
}

void Application::LoadFriends()
{
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
			m_sCurrentFriend = wsValueName;
		}

		//we don't add empty strings (i.e. the default value, if it has a data as null).
		if (*wsValueName) {
			m_friends.push_back(wsValueName);
		}

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
	if (dwError != ERROR_FILE_NOT_FOUND && *wsData) {
		m_sCurrentFriend = wsData;
	}
}

void Application::SaveFriend(const std::wstring& name, const std::wstring& ip)
{
	//we write into the registry the nickname and its associated IP
	DWORD cbData = (name.length() + 1) * 2;
	DWORD dwError = RegSetValueExW(m_hKey, name.data(), 0, REG_SZ, (BYTE*)ip.data(), cbData);
	if (ERROR_SUCCESS != dwError)
	{
		DisplayError(dwError);
	}

	//we set into the registry this nickname as the last nickname the computer was connected to:
	cbData = (name.length() + 1) * 2;
	dwError = RegSetValueExW(m_hKey, 0, 0, REG_SZ, (BYTE*)name.data(), cbData);
	if (ERROR_SUCCESS != dwError)
	{
		DisplayError(dwError);
	}
}

const std::wstring&& Application::GetIpOfFriend(const std::wstring& friend_name)
{
	WCHAR wsIP[31];
	*wsIP = NULL;
	DWORD cbSize = 31;

	DWORD dwError = RegQueryValueExW(m_hKey, friend_name.data(), 0, 0, (BYTE*)wsIP, &cbSize);
	if (ERROR_SUCCESS != dwError)
	{
		if (dwError == ERROR_FILE_NOT_FOUND)
		{
			MessageBox(NULL, L"This nickname does not exist!\n\nPlease select a nickname from the\
								list or write an IP to connect to.", L"Unrecognized nickname!", MB_ICONWARNING);
		}

		else
		{
			DisplayError(dwError);
		}
	}

	return std::wstring(wsIP);
}

void Application::SetLastFriend(const std::wstring& last_friend_name)
{
	//we set into the registry this nickname as the last nickname the computer was connected to:
	DWORD cbData = (last_friend_name.length() + 1) * 2;
	DWORD dwError = RegSetValueExW(m_hKey, 0, 0, REG_SZ, (BYTE*)last_friend_name.data(), cbData);
	if (ERROR_SUCCESS != dwError)
	{
		DisplayError(dwError);
	}
}

const std::wstring&& Application::GetLastFriend() const
{
	//retrieve the last Nickname this computer was connected to:
	DWORD dwNickSize = 20;
	WCHAR wsNick[20];
	DWORD dwError = RegQueryValueExW(m_hKey, 0, 0, 0, (BYTE*)wsNick, &dwNickSize);
	if (0 != dwError && ERROR_FILE_NOT_FOUND != dwError)
	{
		DisplayError(dwError);
	}

	return std::wstring(wsNick);
}