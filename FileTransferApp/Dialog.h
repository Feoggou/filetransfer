#pragma once

#include "Window.h"

class Dialog: public Window
{
public:
	Dialog(HWND hParent, int resource_id) : Window(hParent), m_resourceID(resource_id) {}
	void CreateModal();

	void SetHandle(HWND hWnd) {m_hWnd = hWnd;}

private:
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	virtual void OnInitDialog() = 0;
	virtual void OnDialogProcedure() {}
	virtual void OnClose();
	virtual void OnDestroy();
	virtual void OnCommand(WORD /*code*/, WORD /*id*/, HWND /*hControl*/) {}
	virtual void OnNotify(NMHDR*) {}

private:
	Dialog(const Dialog&);
	Dialog& operator=(const Dialog&);

private:
	const int m_resourceID;
};