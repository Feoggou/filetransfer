#include "ConfirmReceive.h"

#include "General.h"
#include "Application.h"
#include "String.h"
#include "Debug.h"

#include <ShObjIdl.h>
#include <ShlObj.h>


ConfirmReceive::ConfirmReceive(void)
{
}


ConfirmReceive::~ConfirmReceive(void)
{
}

bool ConfirmReceive::operator()(const std::wstring& parent_display_name, std::wstring& wsSavePath, LARGE_INTEGER& liSize)
{
#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
	return vista_confirm(parent_display_name, wsSavePath, liSize);
#else
	return xp_confirm(parent_display_name, wsSavePath, liSize);
#endif
}

#if _WIN32_WINNT >= _WIN32_WINNT_VISTA

bool ConfirmReceive::vista_confirm(const std::wstring& parent_display_name, std::wstring& wsSavePath, LARGE_INTEGER& liSize)
{
	//we retrieve an IFileSaveDialog object
	IFileSaveDialog* pDlg;
	HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, (void**)&pDlg);
	if (FAILED(hr))
	{
		DisplayError(hr);
		return false;
	}

	//we retrieve the PIDL of the desktop: needed to retrieve the desktop shell item
	ITEMIDLIST* pidlDesktop;
	hr = SHGetKnownFolderIDList(FOLDERID_Desktop, 0, 0, &pidlDesktop);
	if (FAILED(hr))
	{
		pDlg->Release();

		DisplayError(hr);
		return false;
	}

	//retrieving the desktop shell item
	IShellItem* pShellItem;
	hr = SHCreateItemFromIDList(pidlDesktop, IID_IShellItem, (void**)&pShellItem);
	if (FAILED(hr))
	{
		pDlg->Release();
		CoTaskMemFree(pidlDesktop);

		DisplayError(hr);
		return false;
	}

	//we don't need the pidl of the desktop anymore
	CoTaskMemFree(pidlDesktop);

	hr = pDlg->SetDefaultFolder(pShellItem);
	hr = pDlg->SetOptions(FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT);
	hr = pDlg->SetFileName(parent_display_name.data());

	BOOL bExit = false;

	do
	{
		//show the dialogbox
		hr = pDlg->Show(theApp->GetMainWindow());
		if (FAILED(hr))
		{
			//either canceled or error
			if (pShellItem)
				pShellItem->Release();
			pDlg->Release();

			if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
				DisplayError(hr);
			return false;
		}

		//we don't need the desktop shell item.
		if (pShellItem)
		{
			pShellItem->Release();
			pShellItem = NULL;
		}

		//we retrieve the shell item of the selected item
		hr = pDlg->GetResult(&pShellItem);
		if (FAILED(hr))
		{
			pDlg->Release();

			DisplayError(hr);
			return false;
		}

		//we retrieve the full file name of the selected item	
		WCHAR* wsFileName;
		hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &wsFileName);
		if (FAILED(hr))
		{
			pShellItem->Release();
			pDlg->Release();

			DisplayError(hr);
			return 0;
		}

		pShellItem->Release();
		pShellItem = NULL;

		int len = StringLen(wsFileName);
		len++;

		wsSavePath = wsFileName;
		CoTaskMemFree(wsFileName);
		//we set the first '\\' to '\0' so that we would check the free space on this disk
		wsSavePath[2] = 0;

		//retrieving the free disk space: we need it to see if we can save the file(s) here
		ULARGE_INTEGER freespace = {0};
		GetDiskFreeSpaceExW(wsSavePath.data(), NULL, NULL, &freespace);
		if (freespace.QuadPart < (ULONGLONG)liSize.QuadPart)
		{
			//ok, not enough disk space: warn and try again

			MessageBox(0, L"There is not enough disk space on the drive to save the file/folder. Try other drive.", L"Disk Space", 0);
			continue;
		}
		else
		{
			//ok, we have enough disk space. the loop ends here.
			pDlg->Release();

			wsSavePath[2] = '\\';
			bExit = true;
		}


	} while (!bExit);

	return true;
}

#else

bool ConfirmReceive::xp_confirm(const std::wstring& parent_display_name, std::wstring& wsSavePath, LARGE_INTEGER& liSize)
{
	bool bConfirmed = 1;

	OPENFILENAMEW ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = theApp->GetMainWindow();
	ofn.hInstance = Application::GetHInstance();
	ofn.lpstrFile = new WCHAR[2000];
	ofn.nMaxFile = 2000;
	StringCopy(ofn.lpstrFile, parent_display_name.data());
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
				
	BOOL bExit = false;
	BOOL bResult;

	//create a GetSaveFileName dialogbox to choose a directory where to save.
	do
	{
		bResult = GetSaveFileNameW(&ofn);
		if (bResult == false)
		{
			DWORD dwError =  CommDlgExtendedError();
			//error or canceled
			if (dwError)
			{
				WCHAR wsError[500];
				LoadStringW(Application::GetHInstance(), dwError, wsError, 500);
				MessageBox(theApp->GetMainWindow(), wsError, 0, MB_ICONERROR);
			}
						
			delete[] ofn.lpstrFile;
			bExit = true;
			bConfirmed = false;
		}

		else
		{
			//we have chosen a directory.
			int len = StringLen(ofn.lpstrFile);
			len++;

			//copy ofn.lpstrFile into wsSavePath
			wsSavePath = ofn.lpstrFile;
			wsSavePath[2] = 0;

			//retrieve the free disk space and check if we have enough disk space to save the file(s)
			ULARGE_INTEGER freespace = {0};
			GetDiskFreeSpaceExW(*wsSavePath, NULL, NULL, &freespace);
			if (freespace.QuadPart < (ULONGLONG)liSize.QuadPart)
			{
				//ok, we don't have enough free space in this disk: warn

				MessageBox(0, L"There is not enough disk space on the drive to save the file/folder. Try other drive.", L"Disk Space", 0);
				continue;
			}
			else
			{
				delete[] ofn.lpstrFile;
				//ok, we have enough free disk space: we save here
				wsSavePath[2] = '\\';
				bExit = true;
			}
		}

	} while (!bExit);

	return bConfirmed;
}

#endif