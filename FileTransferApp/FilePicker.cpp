#include "FilePicker.h"
#include "Application.h"
#include "Debug.h"


FilePicker::FilePicker(HWND hParent)
	: m_hParent(hParent)
{
}


FilePicker::~FilePicker(void)
{
}

std::wstring FilePicker::operator()()
{
#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
	return vista_pick();
#else
	return xp_pick();
#endif
}
#if _WIN32_WINNT >= _WIN32_WINNT_VISTA

std::wstring FilePicker::vista_pick()
{
	//create a IFileOpenDialog object
	IFileOpenDialog* pDlg;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&pDlg);
	if (FAILED(hr))
	{
		DisplayError(hr);
		return L"";
	}

	//we need the ITEMIDLIST of the desktop so we would create a shell item from it
	ITEMIDLIST* pidlDesktop;
	hr = SHGetKnownFolderIDList(FOLDERID_Desktop, 0, 0, &pidlDesktop);
	if (FAILED(hr))
	{
		pDlg->Release();

		DisplayError(hr);
		return L"";
	}

	//create the shell item
	IShellItem* pShellItem;
	hr = SHCreateItemFromIDList(pidlDesktop, IID_IShellItem, (void**)&pShellItem);
	if (FAILED(hr))
	{
		pDlg->Release();
		CoTaskMemFree(pidlDesktop);

		DisplayError(hr);
		return L"";
	}

	//we don't need the ITEMIDLIST anymore, so we free it.
	CoTaskMemFree(pidlDesktop);

	//set the default folder: the desktop
	pDlg->SetDefaultFolder(pShellItem);
	pDlg->SetOptions(FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST); 

	//show the dialogbox
	hr = pDlg->Show(m_hParent);
	//if error or canceled
	if (FAILED(hr))
	{
		pShellItem->Release();
		pDlg->Release();

		if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
			DisplayError(hr);
		return L"";
	}
	//we don't need the desktop shell item, so we release it
	pShellItem->Release();

	//we store the result of the dialogbox in the shell item.
	hr = pDlg->GetResult(&pShellItem);
	if (FAILED(hr))
	{
		pDlg->Release();

		DisplayError(hr);
		return L"";
	}

	//we retrieve the displayname of the chosen item.
	WCHAR* wsFileName;
	pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &wsFileName);

	//we don't need the shell item retrieved and we don't need the dialogbox.
	pShellItem->Release();
	pDlg->Release();

	std::wstring result(wsFileName);
	CoTaskMemFree(wsFileName);

	return result;
}

#else

std::wstring FilePicker::xp_pick()
{
	//we need a GetOpenFileName to retrieve a file
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hParent;
	ofn.hInstance = Application::GetHInstance();
	ofn.lpstrFile = new WCHAR[2000];
	*ofn.lpstrFile = 0;
	ofn.nMaxFile = 2000;
	ofn.Flags = OFN_FILEMUSTEXIST;

	//display the dialogbox and retrieve the result
	if (false == GetOpenFileName(&ofn))
	{
		DWORD dwError =  CommDlgExtendedError();
		//either error or canceled
		if (dwError)
		{
			WCHAR wsError[500];
			LoadStringW(Application::GetHInstance(), dwError, wsError, 500);
			MessageBox(m_hParent, wsError, 0, MB_ICONERROR);
		}
		delete[] ofn.lpstrFile;
		return L"";
	}

	std::wstring result(ofn.lpstrFile);
	delete[] ofn.lpstrFile;

	return result;
}

#endif