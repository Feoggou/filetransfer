#ifndef NICKNAMEDLG_H
#define NICKNAMEDLG_H

#include "General.h"
#include "Dialog.h"
#include "resource.h"

class NickNameDlg: public Dialog
{
private:
	WCHAR*		m_wsText;

public:
	//initializes the CNickNameDlg object.
	NickNameDlg(void):Dialog(IDD_NICKNAME), m_wsText(0) {}
	~NickNameDlg(void);

	//retrieves the text written in the editbox.
	const WCHAR* GetText() {return m_wsText;}

private:
	//the function that processes the messages
	//virtual INT_PTR DoDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void OnOK();
};

#endif//NICKNAMEDLG_H