#pragma once

#ifndef NICKNAMEDLG_H
#define NICKNAMEDLG_H

#include "General.h"

class CNickNameDlg
{
private:
	HWND		m_hDlg;
	WCHAR*		m_wsText;

public:
	CNickNameDlg(void);
	~CNickNameDlg(void);

	INT_PTR DoModal(HWND hParent);
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	//retrieves the text written in the editbox.
	const WCHAR* GetText();
};

#endif//NICKNAMEDLG_H