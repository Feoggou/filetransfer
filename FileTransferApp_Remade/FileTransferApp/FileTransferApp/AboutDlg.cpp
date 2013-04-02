#include "AboutDlg.h"
#include "resource.h"
#include "Socket.h"
#include <Ws2tcpip.h>
#include "App.h"


INT_PTR CAboutDlg::DoDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
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

	return Dialog::DoDlgProc(uMsg, wParam, lParam);
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
	//position 8, because we need to check the first 7 chars.
	char oldchar = *(sHostName + 8);
	*(sHostName + 8) = 0;
	if (0 == StringCompA(sHostName, "192.168."))
	{
		SetDlgItemTextA(theApp.GetHWND(), IDC_ST_IPDESC, "This is a private internal address provided by your router. Your computer can be used as a server only for the computers connected to the same router.");
	}
	else
	{
		SetDlgItemTextA(theApp.GetHWND(), IDC_ST_IPDESC, "This IP Address can be used to create the connection: Your computer can be used as a server to which any other computer can connect.");
	}
	//put back the character that was at this position.
	*(sHostName + 8) = oldchar;

	//sHostName is the IP
	int len = 21 + StringLenA(sHostName);
	string sIP;
	sIP.reserve(len);
	sIP = "Your IP Address is: ";
	sIP += sHostName;

	//show the message with the IP
	SetDlgItemTextA(theApp.GetHWND(), IDC_STATIC_IP, sIP.c_str());

	//we no longer need pResults
	freeaddrinfo(pResults);
}