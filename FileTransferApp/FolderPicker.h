#pragma once

#include "General.h"
#include <string>

class FolderPicker
{
public:
	FolderPicker(HWND hParent);
	~FolderPicker(void);

	std::wstring operator()();

private:

#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
	std::wstring vista_pick();
#else
	std::wstring xp_pick();
#endif

private:
	HWND	m_hParent;
};

