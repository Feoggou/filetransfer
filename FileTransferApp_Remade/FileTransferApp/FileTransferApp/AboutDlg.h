#pragma once

#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include "General.h"
#include "Dialog.h"
#include "resource.h"

class CAboutDlg: public Dialog
{
public:
	//Initializes the CAboutDlg object.
	CAboutDlg():Dialog(IDD_ABOUTBOX) {}

protected:
	//the function that processes the messages.
	virtual INT_PTR DoDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	//Initializes the dialogbox.
	void OnInitDialog();
};

#endif//ABOUTDLG_H