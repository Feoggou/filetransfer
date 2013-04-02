#pragma once

#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include "General.h"

class CAboutDlg
{
private:
	HWND	m_hDlg;

public:
	CAboutDlg(void) {};
	~CAboutDlg(void) {};

	void DoModal(HWND hParent);

private:
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnInitDialog();
};

#endif//ABOUTDLG_H