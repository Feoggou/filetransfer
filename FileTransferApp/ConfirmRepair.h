#pragma once

#include "General.h"
#include <string>

class ConfirmRepair
{
public:
	ConfirmRepair(void);
	~ConfirmRepair(void);

	bool operator()(const std::wstring& parent_display_name, std::wstring& wsSavePath, LARGE_INTEGER& liSize);

private:
#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
	bool vista_repair(const std::wstring& parent_display_name, std::wstring& wsSavePath, LARGE_INTEGER& liSize);
#else
	bool xp_repair(const std::wstring& parent_display_name, std::wstring& wsSavePath, LARGE_INTEGER& liSize);
#endif
};

