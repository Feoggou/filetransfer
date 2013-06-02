#include "ConfirmRepair.h"

#include "General.h"
#include "Application.h"
#include "String.h"
#include "Debug.h"
#include "DoubleList.h"
#include "Tools.h"

#include <ShObjIdl.h>
#include <ShlObj.h>


ConfirmRepair::ConfirmRepair(void)
{
}


ConfirmRepair::~ConfirmRepair(void)
{
}

bool ConfirmRepair::operator()(const std::wstring& parent_display_name, std::wstring& wsSavePath, LARGE_INTEGER& liSize)
{
#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
	return vista_repair(parent_display_name, wsSavePath, liSize);
#else
	return xp_repair(parent_display_name, wsSavePath, liSize);
#endif
}

#if _WIN32_WINNT >= _WIN32_WINNT_VISTA

bool ConfirmRepair::vista_repair(const std::wstring& parent_display_name, std::wstring& wsSavePath, LARGE_INTEGER& liSize)
{
	//REPAIR MODE
	//create an IFileOpenDialog object
	IFileOpenDialog* pDlg;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&pDlg);
	if (FAILED(hr))
	{
		DisplayError(hr);
		return false;
	}

	//retrieve the PIDL of the desktop - needed to get the shell item of the desktop
	ITEMIDLIST* pidlDesktop;
	hr = SHGetKnownFolderIDList(FOLDERID_Desktop, 0, 0, &pidlDesktop);
	if (FAILED(hr))
	{
		pDlg->Release();

		DisplayError(hr);
		return false;
	}

	//retrieve the shell item of the desktop
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
	pDlg->SetDefaultFolder(pShellItem);
	pDlg->SetOptions(FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PICKFOLDERS); 
	pDlg->SetFileName(parent_display_name.data());

	BOOL bExit = false;
	do
	{
		//we display the dialogbox
		hr = pDlg->Show(theApp->GetMainWindow());
		if (FAILED(hr))
		{
			//canceled or error
			if (pShellItem) pShellItem->Release();
			pDlg->Release();

			if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
				DisplayError(hr);
			return false;
		}
		//we don't need the shell item of the desktop
		if (pShellItem) pShellItem->Release();

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
			return false;
		}

		IShellFolder* pFolder;
		hr = pShellItem->BindToHandler(NULL, BHID_SFObject, IID_IShellFolder, (void**)&pFolder);
		if (FAILED(hr))
		{
			CoTaskMemFree(wsFileName);
			pShellItem->Release();
			pDlg->Release();

			DisplayError(hr);
			return false;
		}

		pShellItem->Release();
		pShellItem = NULL;

		int len = StringLen(wsFileName);
		len++;

		wsSavePath = wsFileName;
		CoTaskMemFree(wsFileName);
		wsSavePath[2] = 0;

		LARGE_INTEGER liExistingSize = {0};

		//checking the freespace
		ULARGE_INTEGER freespace = {0};
		GetDiskFreeSpaceExW(wsSavePath.data(), NULL, NULL, &freespace);
		CDoubleList<FILE_ITEM> Items(OnDestroyFileItemCoTask);
		wsSavePath[2] = '\\';

		SearchFolder(pFolder, Items, liExistingSize);
		pFolder->Release();

		//nu luam in considerare fisierele care exista deja.
		if (liSize.QuadPart >= liExistingSize.QuadPart)
		{
			if (freespace.QuadPart < (ULONGLONG)liSize.QuadPart - liExistingSize.QuadPart)
			{
				MessageBox(0, L"There is not enough disk space on the drive to save the file/folder. Try other drive.", L"Disk Space", 0);
			}
			else 
			{
				pDlg->Release();
				bExit = true;
			}
		}
		else
		{
			if (freespace.QuadPart < (ULONGLONG)liSize.QuadPart)
			{
				MessageBox(0, L"There is not enough disk space on the drive to save the file/folder. Try other drive.", L"Disk Space", 0);
			}
			else 
			{
				pDlg->Release();
				bExit = true;
			}
		}

	}while (!bExit);

	return true;
}

#else

bool ConfirmRepair::xp_repair(const std::wstring& parent_display_name, std::wstring& wsSavePath, LARGE_INTEGER& liSize)
{
	//REPAIR MODE!!
	bool bRetValue = true;

	BROWSEINFOW bi = {0};
	bi.hwndOwner = theApp->GetMainWindow();
	bi.lpszTitle = L"Chose the folder you wish to repair:";
	bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_NONEWFOLDERBUTTON;
	bi.pszDisplayName = new WCHAR[MAX_PATH];
			
	ITEMIDLIST* pidlResult;
	BOOL bExit = false;

	do
	{
		pidlResult = SHBrowseForFolderW(&bi);
		if (pidlResult != 0)
		{
			IShellItem* pShellItem;
			HRESULT hr = SHCreateShellItem(0, 0, pidlResult, &pShellItem);
			if (FAILED(hr))
			{
				CoTaskMemFree(pidlResult);

				DisplayError(hr);
				bRetValue = false;
				break;
			}

			WCHAR* wsFullName;
			hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &wsFullName);
			if (FAILED(hr))
			{
				CoTaskMemFree(pidlResult);
				pShellItem->Release();

				MessageBox(theApp->GetMainWindow(), L"Invalid selection. Chose a directory or a drive!", L"Invalid selection!", MB_ICONWARNING);
				continue;
			}

			CoTaskMemFree(pidlResult);
			pShellItem->Release();

			int len = StringLen(wsFullName);
			len++;
			
			wsSavePath = wsFullName;
			CoTaskMemFree(wsFullName);
			wsSavePath[2] = 0;

			LARGE_INTEGER liExistingSize = {0};

			//checking the freespace
			ULARGE_INTEGER freespace = {0};
			GetDiskFreeSpaceExW(wsSavePath.data(), NULL, NULL, &freespace);
			CDoubleList<FILE_ITEM> Items(OnDestroyFileItemCoTask);
			wsSavePath[2] = '\\';

			IShellFolder* pDesktop;
			hr = SHGetDesktopFolder(&pDesktop);
			if (FAILED(hr))
			{
				DisplayError(hr);
				bRetValue = false;
				break;
			}

			ITEMIDLIST* pidlFolder;
			WCHAR wsDisplayName[260];
			hr = pDesktop->ParseDisplayName(theApp->GetMainWindow(), NULL, wsDisplayName, NULL, &pidlFolder, NULL);
			if (FAILED(hr))
			{
				pDesktop->Release();

				DisplayError(hr);
				bRetValue = false;
				break;
			}

			wsSavePath = wsDisplayName;

			IShellFolder* pFolder;
			hr = pDesktop->BindToObject(pidlFolder, NULL, IID_IShellFolder, (void**)&pFolder);
			if (FAILED(hr))
			{
				pDesktop->Release();
				CoTaskMemFree(pidlFolder);

				DisplayError(hr);
				bRetValue = false;
				break;
			}

			pDesktop->Release();
			CoTaskMemFree(pidlFolder);

			SearchFolder(pFolder, Items, liExistingSize);
			pFolder->Release();

			//nu luam in considerare fisierele care exista deja.
			if (liSize.QuadPart >= liExistingSize.QuadPart)
			{
				if (freespace.QuadPart < (ULONGLONG)liSize.QuadPart - liExistingSize.QuadPart)
				{
					MessageBox(0, L"There is not enough disk space on the drive to save the file/folder. Try other drive.", L"Disk Space", 0);
				}
				else 
				{
					bExit = true;
				}
			}
			else
			{
				if (freespace.QuadPart < (ULONGLONG)liSize.QuadPart)
				{
					MessageBox(0, L"There is not enough disk space on the drive to save the file/folder. Try other drive.", L"Disk Space", 0);
				}
				else 
				{
					bExit = true;
				}
			}
		}
		else bExit = true;

	}while (!bExit);

	delete[] bi.pszDisplayName;

	return bRetValue;
}

#endif