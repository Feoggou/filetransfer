#ifndef DIALOG_H
#define DIALOG_H

#include "General.h"
#include "Exceptions.h"
#include "Mutex.h"

//an abstract class, parent for all dialogs
class Dialog abstract
{
	//PROTECTED DATA:
protected:
	//the HWND of the dialog
	HWND		m_hDlg;

	//PRIVATE DATA:
private:
	//the resource ID of the dialog, from resources.h
	const UINT	m_nIDD;
	//a mutex: used for the static AlreadyExists function
	Mutex		m_alreadyExistsMutex;

	//PUBLIC FUNCTIONS
public:
	//throws an Exception if an instance of this dialog kind has already been created
	Dialog(UINT uID);
	virtual ~Dialog(void)=0 {}

	HWND GetHWND() {return m_hDlg;}
	//for automatic conversion from Dialog to HWND to be used in functions
	operator HWND() {return m_hDlg;}
	//Creates the dialog. Throws a WindowException on failure
	//This is meant for ANY derived class!
	INT_PTR DoModal(HWND hParent);
	//returns the corresponding Dialog* for a given HWND dialog. returns 0 if no Dialog* is associated with the HWND
	//the association is set up using SetWindowLongPtrW.
	static Dialog* GetPtrToDialog(HWND hDialog);

private:
	//the main message callback. it calls the virtual DoDlgProc.
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	//PROTECTED FUNCTIONS
protected:
	//called by Dialog::DlgProc. Any derived class implementing DoDlgProc should call at the end Dialog::DoDlgProc
	virtual INT_PTR DoDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void OnInitDialog(){}
	virtual void OnClose() {EndDialog(m_hDlg, IDCANCEL);}
	virtual void OnDestroy() {EndDialog(m_hDlg, IDCANCEL);}
	virtual void OnOK() {EndDialog(m_hDlg, IDOK);}
	virtual void OnCancel() {EndDialog(m_hDlg, IDCANCEL);}

	//PRIVATE FUNCTIONS
private:
	template <typename DialogClass>
		bool AlreadyExists(DialogClass* dialog);
};





//IMPLEMENTATION

template <typename DialogClass>
inline bool Dialog::AlreadyExists(DialogClass* dialog)
{
	m_alreadyExistsMutex.Lock();
	static int count = 0;
	
	return (++count > 1);
}

inline Dialog* Dialog::GetPtrToDialog(HWND hDialog)
{
	return reinterpret_cast<Dialog*>(GetWindowLongPtrW(hDialog, GWL_USERDATA));
}

#endif//DIALOG_H