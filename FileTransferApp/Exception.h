#pragma once

#include <exception>

class WindowsException: public std::exception
{
public:
	WindowsException(DWORD dwError, const char* func_name, const char* file_name, int line)
	{
		//TODO
	}

	const char* what() const override /*noexcept*/
	{
		//TODO
		return "";
	}
};

#define THROW(TClass, TObj) throw TClass(TObj, __FUNCTION__, __FILE__, __LINE__)