#pragma once

#include "General.h"
#include "Dialog.h"
#include "resource.h"

class AboutDlg: public Dialog
{
public:
	AboutDlg(HWND hParent): Dialog(hParent, IDD_ABOUTBOX) {}
	~AboutDlg() {}

private:
	AboutDlg(const AboutDlg&);

private:
	void OnInitDialog() override;
	void OnCommand(WORD code, WORD id, HWND hControl) override;
	void OnNotify(NMHDR*) override;
};