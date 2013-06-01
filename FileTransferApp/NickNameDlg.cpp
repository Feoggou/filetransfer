#include "NickNameDlg.h"
#include "resource.h"

#include "Application.h"


CNickNameDlg::CNickNameDlg(void)
{
	m_wsText = NULL;
}


CNickNameDlg::~CNickNameDlg(void)
{
	if (m_wsText)
	{
		delete[] m_wsText;
		m_wsText = 0;
	}
}

INT_PTR CNickNameDlg::DoModal(HWND hParent)
{
	INT_PTR nResult = DialogBoxParamW(Application::GetHInstance(), MAKEINTRESOURCE(IDD_NICKNAME), hParent, DlgProc, (LPARAM)this);
	if (nResult == -1)
		DisplayError();

	return nResult;
}

INT_PTR CALLBACK CNickNameDlg::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CNickNameDlg* pThis = NULL;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			pThis = (CNickNameDlg*)lParam;
			pThis->m_hDlg = hDlg;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, IDCANCEL);
		break;

	case WM_DESTROY:
		EndDialog(hDlg, IDCANCEL);
		break;

	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				{
					//retrieve the IDCE_NICKNAME editbox
					HWND hEdit = GetDlgItem(hDlg, IDCE_NICKNAME);

					//retrieve the length of the text written there
					int len = GetWindowTextLengthW(hEdit);

					if (len > 10)
					{
						MessageBox(hDlg, L"The nickname is too long!\n\nPlease write a nickname of maximum 10\
 characters.", L"Invalid Nickname", MB_ICONWARNING);
						break;
					}

					else if (len == 0)
					{
						MessageBox(hDlg, L"You did not write any nickname!\n\nPlease write a nickname and press OK.\n\
Or, you can cancel the operation by pressing the X button in the top right corner of the box!", L"Invalid Nickname", MB_ICONWARNING);
						break;
					}

					else if (len < 3)
					{
						MessageBox(hDlg, L"The nickname is too short!\n\nPlease write a nickname of minimum 3\
 characters.", L"Invalid Nickname", MB_ICONWARNING);
						break;
					}

					len++;
					pThis->m_wsText = new WCHAR[len];
					GetWindowTextW(hEdit, pThis->m_wsText, len);

					BOOL bIsGood = true;

					if (!iswalpha(*pThis->m_wsText))
					{
						MessageBox(hDlg, L"The first character in the nickname must be a letter!", L"Invalid Nickname", MB_ICONWARNING);
							break;
					}

					int i = 0;
					for (i = 0; i < len - 1; i++)
					{
						if (!iswalnum(*(pThis->m_wsText + i)) && *(pThis->m_wsText + i) != ' ')
						{
							bIsGood = false;
							MessageBox(hDlg, L"The nickname must contain only letters and digits! The nickname can also contain spaces between the letters and the digits.", L"Invalid Nickname", MB_ICONWARNING);
							break;
						}
					}
					if (!bIsGood) break;

					i--;
					if (*(pThis->m_wsText + i) == ' ')
					{
						MessageBox(hDlg, L"The nickname cannot end with a space!", L"Invalid Nickname", MB_ICONWARNING);
							break;
					}

					//if OK was pressed, OK we return!
					EndDialog(hDlg, IDOK);
				}
				break;
			}
		}
		break;
	}

	return 0;
}

const WCHAR* CNickNameDlg::GetText()
{
	return m_wsText;
}