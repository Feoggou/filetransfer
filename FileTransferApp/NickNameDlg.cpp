#include "NickNameDlg.h"
#include "resource.h"

#include "Application.h"

NickNameDlg::~NickNameDlg(void)
{
	/*if (m_wsText)
	{
		delete[] m_wsText;
		m_wsText = 0;
	}*/
}

void NickNameDlg::OnOK()
{
	//retrieve the IDCE_NICKNAME editbox
	HWND hEdit = GetDlgItem(m_hWnd, IDCE_NICKNAME);

	//retrieve the length of the text written there
	int len = GetWindowTextLengthW(hEdit);

	if (len > 10)
	{
		MessageBox(m_hWnd, L"The nickname is too long!\n\nPlease write a nickname of maximum 10\
						  characters.", L"Invalid Nickname", MB_ICONWARNING);
		return;
	}

	else if (len == 0)
	{
		MessageBox(m_hWnd, L"You did not write any nickname!\n\nPlease write a nickname and press OK.\n\
						  Or, you can cancel the operation by pressing the X button in the top right corner of the box!", L"Invalid Nickname", MB_ICONWARNING);
		return;
	}

	else if (len < 3)
	{
		MessageBox(m_hWnd, L"The nickname is too short!\n\nPlease write a nickname of minimum 3\
						  characters.", L"Invalid Nickname", MB_ICONWARNING);
		return;
	}

	len++;
	m_wsText.reset(new WCHAR[len]);
	GetWindowTextW(hEdit, m_wsText.get(), len);

	BOOL bIsGood = true;

	if (!iswalpha(*m_wsText.get()))
	{
		MessageBox(m_hWnd, L"The first character in the nickname must be a letter!", L"Invalid Nickname", MB_ICONWARNING);
		return;
	}

	int i = 0;
	for (i = 0; i < len - 1; i++)
	{
		if (!iswalnum(*(m_wsText.get() + i)) && *(m_wsText.get() + i) != ' ')
		{
			bIsGood = false;
			MessageBox(m_hWnd, L"The nickname must contain only letters and digits! The nickname can also contain spaces between the letters and the digits.", L"Invalid Nickname", MB_ICONWARNING);
			return;
		}
	}
	if (!bIsGood) return;

	i--;
	if (*(m_wsText.get() + i) == ' ')
	{
		MessageBox(m_hWnd, L"The nickname cannot end with a space!", L"Invalid Nickname", MB_ICONWARNING);
		return;
	}

	//if OK was pressed, OK we return!
	EndDialog(m_hWnd, IDOK);
}