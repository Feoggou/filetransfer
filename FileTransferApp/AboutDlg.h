#pragma once

#include "General.h"
#include "Dialog.h"
#include "resource.h"

class CAboutDlg: public Dialog
{
public:
	CAboutDlg(HWND hParent): Dialog(hParent, IDD_ABOUTBOX) {}
	~CAboutDlg() {}

private:
	CAboutDlg(const CAboutDlg&);

private:
	void OnInitDialog() override;
	void OnCommand(WORD code, WORD id, HWND hControl) override;
	void OnNotify(NMHDR*) override;
};