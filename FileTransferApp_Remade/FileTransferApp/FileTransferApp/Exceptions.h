#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "General.h"
#include "Events.h"

namespace Exceptions
{
	//CLASS Exception
	//base exception handler. cannot be instantiated.
	//It uses the error code, the error string and the error description.
	//also the file where the log is written.
	class Exception: public exception// abstract
	{
		//these will be accessible by the children classes only
	protected:
		string				m_sErrorMsg;
		//wstring				m_wsWhere;

//		static HANDLE		m_hFile;

		//we cannot create an Exception object directly. it is used only by children.
	public:
		Exception(const string& sWhat, const string& sFile, int nLine);
		//default ctor: it doesn't initialize m_sErrorMsg
		Exception();
		~Exception();

		virtual const char* what() const;

		//void SetError(const string& sError)	{m_sError = sError;}
	private:
		void InitializeFile();
		static ofstream m_LogFile;

		friend void ReportError(const char* sWhat, const char* sUserMsg);
	};

	//CLASS WindowException
	//handles the windows errors. we don't want the user of this class to access public members of exception,
	class WindowException: public Exception
	{
	protected:
		DWORD				m_dwError;
	public:
		//with no error code: we find it here
		//sWhere contains an optional message regarding where it happened.
		WindowException(const string& sFile, int nLine, DWORD dwError = 0);
		//WindowException operator=(const WindowException& obj);
	};

	//CLASS SocketException
	//handles the exceptions regarding sockets
	class SocketException: public WindowException
	{
		Events::NetworkEvent*	m_pNetworkEvent;

	public:
		SocketException(const string& sFile, int nLine, DWORD dwError = 0);
		~SocketException();

		Events::NetworkEvent GetEvent() {return *m_pNetworkEvent;}
	};

	class FileException: public WindowException
	{
	};

	//NON-MEMBER FUNCTIONS
	void ReportError(const char* sWhat, const char* sUserMsg);
}


//if the function returns 0, it throws an exception
#define WN(x)									\
{												\
	if (false == (x))							\
		throw Exceptions::WindowException(__FILE__, __LINE__);	\
}		

//if the function returns the windows error, it throws that specific exception
#define WNE(x)									\
{												\
	DWORD dwError = (x);						\
	if (dwError)								\
	throw Exceptions::WindowException(__FILE__, __LINE__, dwError);	\
}

//throw a WindowException if "condition" is satisfied
#define ThrowWinIf(condition)														\
{																					\
	if ((condition))																\
		throw Exceptions::WindowException(__FILE__, __LINE__, GetLastError());		\
}

//throw an Exception if "condition" is satisfied. the error message is "what".
#define ThrowIf(condition, what)									\
{																	\
	if ((condition))												\
		throw Exceptions::Exception(what, __FILE__, __LINE__);		\
}

//throws a WindowException with the dwError error code.
#define ThrowWin(dwError)													\
{																			\
	throw Exceptions::WindowException(__FILE__, __LINE__, dwError);		\
}


#endif//EXCEPTIONS_H