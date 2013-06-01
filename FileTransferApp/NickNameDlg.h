#pragma once

#include "General.h"
#include "Dialog.h"
#include "resource.h"

#include <memory>

class NickNameDlg : public Dialog
{
private:
	std::unique_ptr<WCHAR[]>		m_wsText;

public:
	NickNameDlg(HWND hParent): Dialog(hParent, IDD_NICKNAME) {m_wsText = nullptr;}
	~NickNameDlg(void);

	//retrieves the text written in the editbox.
	const WCHAR* GetText() const {return m_wsText.get();}

private:
	void OnOK() override;
};