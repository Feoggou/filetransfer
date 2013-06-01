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
	void OnOK() override;
	void OnNotify(NMHDR*) override;
};