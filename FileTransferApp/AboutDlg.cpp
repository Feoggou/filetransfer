#include "AboutDlg.h"
#include "resource.h"
#include "SamSocket.h"
#include <Ws2tcpip.h>

#include "Application.h"

void CAboutDlg::DoModal(HWND hParent)
{
	INT_PTR nResult = DialogBoxParamW(Application::GetHInstance(),MAKEINTRESOURCE(IDD_ABOUTBOX), hParent, DlgProc, (LPARAM)this);
	if (nResult == -1)
		DisplayError();
}

INT_PTR CALLBACK CAboutDlg::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CAboutDlg* pThis = NULL;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			//save pThis for further use and save hDlg for use in the class functions
			pThis = (CAboutDlg*)lParam;
			pThis->m_hDlg = hDlg;
			pThis->OnInitDialog();
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, IDCANCEL);
		break;

	case WM_DESTROY:
		EndDialog(hDlg, IDCANCEL);
		break;

	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				{
					EndDialog(hDlg, IDCANCEL);
				}
				break;
			}
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR* pNMHDR = (NMHDR*)lParam;
			switch (pNMHDR->code)
			{
				//when the user has clicked the link
			case NM_CLICK:
				{
					SHELLEXECUTEINFO sei;
					ZeroMemory(&sei,sizeof(SHELLEXECUTEINFO));
					sei.cbSize = sizeof(SHELLEXECUTEINFO);
					sei.lpVerb = TEXT( "open" );
					sei.lpFile = L"http://compose.mail.yahoo.com/?To=fio_244@yahoo.com";	
					sei.nShow = SW_SHOWNORMAL;

					ShellExecuteEx(&sei);
				}
				break;
			}
		}
		break;
	}

	return 0;
}

void CAboutDlg::OnInitDialog()
{
	//showing the ip of the user
	char sHostName[300];
	//retrieving the host name
	if (SOCKET_ERROR == gethostname(sHostName, 300)) return;

	ADDRINFOA hints = {0}, *pResults;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//retrieving the address info for the host name and port 14147
	DWORD dwResult = getaddrinfo(sHostName, "14147", &hints, &pResults);
	if (0 != dwResult) return;
	for (ADDRINFOA* X = pResults; X != NULL; X = pResults->ai_next)
	{
		sockaddr_in* sockul = (sockaddr_in*)X->ai_addr;
		strcpy_s(sHostName, 300, inet_ntoa(sockul->sin_addr));
	}

	//save the character of position 8, because we will make it null in the string
	char oldchar = *(sHostName + 8);
	*(sHostName + 8) = 0;
	if (0 == StringCompA(sHostName, "192.168."))
	{
		SetDlgItemTextA(m_hDlg, IDC_ST_IPDESC, "This is a private internal address provided by your router. Your computer can be used as a server only for the computers connected to the same router.");
	}
	else
	{
		SetDlgItemTextA(m_hDlg, IDC_ST_IPDESC, "This IP Address can be used to create the connection: Your computer can be used as a server to which any other computer can connect.");
	}
	//put back the character that was at this position.
	*(sHostName + 8) = oldchar;

	//sHostName is the IP
	int len = 21 + StringLenA(sHostName);
	char* sIP = new char[len];
	StringCopyA(sIP, "Your IP Address is: ");
	StringCopyA(sIP + 20, sHostName);

	//show the message with the IP
	SetDlgItemTextA(m_hDlg, IDC_STATIC_IP, sIP);
	delete[] sIP;

	//we no longer need pResults
	freeaddrinfo(pResults);
}