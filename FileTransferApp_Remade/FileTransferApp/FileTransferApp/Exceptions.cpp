#include <algorithm>
#include <fstream>

#include "General.h"
#include "Exceptions.h"

#include "App.h"
#include "Utils.h"

#include "Events.h"

using namespace Exceptions;		

Exception::Exception(const string& sWhat, const string& sFile, int nLine)
{
	//constructing the m_sErrorMsg string.
	m_sErrorMsg.reserve(sWhat.length() + sFile.length() + 15);
	m_sErrorMsg = "ERROR: ";
	m_sErrorMsg += sWhat;
	m_sErrorMsg += '\n';
	m_sErrorMsg += sFile;
	m_sErrorMsg += ", line ";
	m_sErrorMsg += to_string(static_cast<_Longlong>(nLine));

	InitializeFile();
}

Exception::Exception()
{
	InitializeFile();
}

void Exception::InitializeFile()
{
	DWORD dwError = GetLastError();

	////we open the file only once
	//if (m_hFile != INVALID_HANDLE_VALUE)
	//{
	//	m_hFile = CreateFileA("errors.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL);
	//	if (m_hFile == INVALID_HANDLE_VALUE)
	//	{
	//		MessageBox(theApp.GetHWND(), L"The error file could not be created", 0, MB_ICONERROR);
	//		return;
	//	}

	//	SetFilePointer(m_hFile, 0, 0, FILE_END);
	//}
	if (!m_LogFile.is_open())
	{
		m_LogFile.open("errors.log", ios_base::out | ios_base::app, _SH_DENYWR);
		if (m_LogFile.fail())
		{
			MessageBox(theApp.GetHWND(), L"The error file could not be created!", 0, MB_ICONERROR);
			return;
		}
	}

	//if the file is already opened, we don't want GetLastError() to say that.
	SetLastError(dwError);
}

Exception::~Exception()
{
	/*if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}*/
}

const char* Exception::what() const
{
	return m_sErrorMsg.c_str();
}

void Exceptions::ReportError(const char* what, const char* sUserMsg)
{
	string sError;
	if (sUserMsg && strlen(sUserMsg))
	{
		sError = sUserMsg;
		sError += '\n';
	}
	sError += what;

#ifdef _DEBUG
	//in debug mode, we display on the debug window and display an error dialog too
	//all the message
	MessageBoxA(0, sError.c_str(), "Error!", MB_ICONERROR);
	//in the debug window we will need this BIG, in order to see it easily
	for_each(sError.begin(), sError.end(), ToUpper()); 
	OutputDebugStringA(sError.c_str());

	//in debug mode, we do not catch anything here:
	throw;
#else
	//in retail mode, we write in a file the error message.
	//we also write in the file the exact time the error occured.

	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);

	//use an auxiliary string to hold, first the time & date, second the error code.
	char sAux[25];
	sprintf(sAux, "%02d-%02d-%04d %02d:%02d:%02d ", sysTime.wDay, sysTime.wMonth, sysTime.wYear,
		sysTime.wHour, sysTime.wMinute, sysTime.wSecond);

	DWORD dwWritten;
	Exception::m_LogFile<<sAux;
	//WriteFile(Exceptions::Exception::m_hFile, sAux, strlen(sAux), &dwWritten, NULL);

	/*sprintf(sAux, " (0x%08u)", m_dwError);
	m_sError += sAux;
	if (!m_sWhere.empty())
	{
		m_sError += " at ";
		m_sError += m_sWhere;
	}

	m_sError += "\r\n";*/
	Exception::m_LogFile<<sError<<endl;
	//WriteFile(Exceptions::Exception::m_hFile, sError.c_str(), sError.length(), &dwWritten, NULL);

	/*m_dwError = 0;
	m_sError.clear();*/

	MessageBoxA(theApp.GetHWND(), sUserMsg, "Error!", MB_ICONERROR);

#endif
}

//WindowException functions

WindowException::WindowException(const string& sFile, int nLine, DWORD dwError)
{
	/*m_sErrorMsg.reserve(sWhat.length() + sFile.length() + 15);
	m_sErrorMsg = "ERROR: ";
	m_sErrorMsg += sWhat;
	m_sErrorMsg += '\n';
	m_sErrorMsg += sFile;
	m_sErrorMsg += ", line ";
	m_sErrorMsg += to_string(static_cast<_Longlong>(nLine));*/

	//we retrieve the error
	if (dwError) m_dwError = dwError;
	else m_dwError = GetLastError();
	char sCode[15];
	sprintf(sCode, " (0x%08X)", m_dwError);

	//this should call "LocalFree" because it allocates the buffer itself.
	char* sBuffer = NULL;
	DWORD dwResult = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, m_dwError, 0, (char*)&sBuffer, 1, 0);
	if (0 == dwResult)
	{
		char sError[30];
		sprintf(sError, "Could not format the error message. Error code: 0x%08X", GetLastError());
		MessageBoxA(hMainWnd, sError, 0, MB_ICONERROR);
		return;
	}
		
	//constructing the m_sErrorMsg string.
	int pos = strlen(sBuffer) - 2;
	*(sBuffer + pos) = 0;
	m_sErrorMsg = "ERROR: ";
	m_sErrorMsg += sBuffer;
	m_sErrorMsg += sCode;
	m_sErrorMsg += '\n';
	m_sErrorMsg += sFile;
	m_sErrorMsg += ", line ";
	m_sErrorMsg += to_string(static_cast<_Longlong>(nLine));
	//here we call LocalFree:
	LocalFree(sBuffer);
}

SocketException::SocketException(const string& sFile, int nLine, DWORD dwError):
WindowException(sFile, nLine, dwError)
{
	using namespace Events;

	switch (m_dwError)
	{
	case 0: m_pNetworkEvent = new NetworkEvent(NetworkEvent::ConnGracefullyClosed);
		break;

	default:
		m_pNetworkEvent = new NetworkEvent(NetworkEvent::ConnBroken);
	}
}

SocketException::~SocketException()
{
	if (m_pNetworkEvent)
	{
		delete m_pNetworkEvent;
		m_pNetworkEvent = 0;
	}
}

//HANDLE Exception::m_hFile = NULL;
ofstream Exception::m_LogFile;