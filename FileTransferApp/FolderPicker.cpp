#include "FolderPicker.h"


FolderPicker::FolderPicker(HWND hParent)
	: m_hParent(hParent)
{
}


FolderPicker::~FolderPicker(void)
{
}

std::wstring FolderPicker::operator()()
{
#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
	return vista_pick();
#else
	return xp_pick();
#endif
}

#if _WIN32_WINNT >= _WIN32_WINNT_VISTA

std::wstring FolderPicker::vista_pick()
{
	//we create a IFileOpenDialog object
	IFileOpenDialog* pDlg;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&pDlg);
	if (FAILED(hr))
	{
		DisplayError(hr);
		return L"";
	}

	//we retrieve the ITEMIDLIST of the desktop, so we would get its shell item
	ITEMIDLIST* pidlDesktop;
	hr = SHGetKnownFolderIDList(FOLDERID_Desktop, 0, 0, &pidlDesktop);
	if (FAILED(hr))
	{
		pDlg->Release();

		DisplayError(hr);
		return L"";
	}

	//we retrieve the shell item of the desktop
	IShellItem* pShellItem;
	hr = SHCreateItemFromIDList(pidlDesktop, IID_IShellItem, (void**)&pShellItem);
	if (FAILED(hr))
	{
		pDlg->Release();
		CoTaskMemFree(pidlDesktop);

		DisplayError(hr);
		return L"";
	}

	//ok, we don't need the ITEMIDLIST anymore, so we free it.
	CoTaskMemFree(pidlDesktop);

	//we set the desktop as the default directory.
	pDlg->SetDefaultFolder(pShellItem);
	pDlg->SetOptions(FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PICKFOLDERS); 

	//show the dialogbox and retrieve the result
	hr = pDlg->Show(m_hParent);
	//error or canceled
	if (FAILED(hr))
	{
		pShellItem->Release();
		pDlg->Release();

		if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
			DisplayError(hr);
		return L"";
	}
	//we don't need the desktop shell item anymore
	pShellItem->Release();

	//we retrieve the selected item as shell item
	hr = pDlg->GetResult(&pShellItem);
	if (FAILED(hr))
	{
		pDlg->Release();

		DisplayError(hr);
		return L"";
	}

	//we retrieve the full file name of the selected item
	WCHAR* wsFileName;
	pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &wsFileName);

	//we don't need the selected shell item and the dialogbox anymore
	pShellItem->Release();
	pDlg->Release();

	std::wstring result = wsFileName;
	CoTaskMemFree(wsFileName);

	return result;
}

#else

std::wstring FolderPicker::xp_pick()
{
	//we need Shell Browser for this.
	BROWSEINFOW bi = {0};
	bi.hwndOwner = m_hParent;
	bi.lpszTitle = L"Chose the folder you wish to send:";
	bi.ulFlags = BIF_EDITBOX;
	bi.pszDisplayName = new WCHAR[MAX_PATH];

	std::wstring result;

	//the result is stored in pidlResult
	ITEMIDLIST* pidlResult;
try_again:
	pidlResult = SHBrowseForFolderW(&bi);
	//if a result
	if (pidlResult != 0)
	{
		//we retrieve the item as a shellitem so we would get its displayname
		IShellItem* pShellItem;
		HRESULT hr = SHCreateShellItem(0, 0, pidlResult, &pShellItem);
		if (FAILED(hr))
		{
			delete[] bi.pszDisplayName;
			CoTaskMemFree(pidlResult);

			DisplayError(hr);
			return L"";
		}

		//we retrieve the filesyspath of the selected item. failure if the selected item
		//is not part of the FILE SYSTEM!
		WCHAR* wsFullName;
		hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &wsFullName);
		if (FAILED(hr))
		{
			CoTaskMemFree(pidlResult);
			pShellItem->Release();

			MessageBox(m_hParent, L"Invalid selection. Chose a directory or a drive!", L"Invalid selection!", MB_ICONWARNING);
			goto try_again;
		}

		//we don't need the shell item anymore
		pShellItem->Release();

		result = wsFullName;
		CoTaskMemFree(wsFullName);

		return result;

	}
	//we don't need the displayname and the pidlResult anymore
	delete[] bi.pszDisplayName;
	if (pidlResult) CoTaskMemFree(pidlResult);
}

#endif