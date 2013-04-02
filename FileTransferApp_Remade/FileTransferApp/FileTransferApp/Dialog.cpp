#include "Dialog.h"
#include "Exceptions.h"
#include "App.h"


//IMPLEMENTATION
Dialog::Dialog(UINT uID): m_nIDD(uID)
{
	m_hDlg = NULL;
	ThrowIf(AlreadyExists(this), "An instance of this dialog already exists!");
}

INT_PTR CALLBACK Dialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Dialog* pThis = GetPtrToDialog(hDlg);

	if (!pThis)
	{
		if (uMsg == WM_INITDIALOG)
		{
			SetWindowLongPtr(hDlg, GWL_USERDATA, lParam);
			(reinterpret_cast<Dialog*>(lParam))->m_hDlg = hDlg;
		}

		return 0;
	}
	
	else
	{
		if (uMsg == WM_INITDIALOG)
		{
			pThis->m_hDlg = hDlg;
		}
		else
		{
			return pThis->DoDlgProc(uMsg, wParam, lParam);
		}
	}
}

INT_PTR Dialog::DoDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			OnInitDialog();
		}
		break;

	case WM_CLOSE:
		OnClose();
		break;

	case WM_DESTROY:
		OnDestroy();
		break;

	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				{
					OnOK();
				}
				break;

			case IDCANCEL:
				{
					OnCancel();
				}
				break;
			}
		}
		break;
	}

	return 0;
}

INT_PTR Dialog::DoModal(HWND hParent)
{
	INT_PTR nResult = DialogBoxParamW(theApp.m_hInst, MAKEINTRESOURCE(m_nIDD), hParent, DlgProc, (LPARAM)this);
	ThrowWinIf(nResult == -1);

	return nResult;
}