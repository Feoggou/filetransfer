#include "Dialog.h"
#include "Application.h"

#include "General.h"

#include <set>
#include <algorithm>

void Dialog::CreateModal()
{
	INT_PTR nResult = DialogBoxParamW(Application::GetHInstance(), MAKEINTRESOURCE(m_resourceID), m_hParent, DialogProc, (LPARAM)this);
	if (nResult == -1)
		DisplayError();
}

INT_PTR CALLBACK Dialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static std::set<Dialog*> dialogs;
	Dialog* pThis = nullptr;

	if (uMsg == WM_INITDIALOG) {
		//save pThis for further use and save hDlg for use in the class functions
		Dialog* pThis = (Dialog*)lParam;
		pThis->SetHandle(hDlg);
		dialogs.insert(pThis);
	} else {
		std::set<Dialog*>::iterator i = std::find(std::begin(dialogs), std::end(dialogs), pThis);
		_ASSERT(i != std::end(dialogs));

		pThis = *i;
	}

	switch (uMsg)
	{
	case WM_INITDIALOG: pThis->OnInitDialog(); break;
	case WM_CLOSE: pThis->OnClose(); break;
	case WM_DESTROY: pThis->OnDestroy(); break;
	case WM_COMMAND: pThis->OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam); break;

	case WM_NOTIFY:
		{
			NMHDR* pNMHDR = (NMHDR*)lParam;
			pThis->OnNotify(pNMHDR);	
		}
		break;
	}

	return 0;
}

void Dialog::OnClose()
{
	_ASSERT(m_hWnd);
	EndDialog(m_hWnd, IDCANCEL);
}

void Dialog::OnDestroy()
{
	_ASSERT(m_hWnd);
	EndDialog(m_hWnd, IDCANCEL);
}