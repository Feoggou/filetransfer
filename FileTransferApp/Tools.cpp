#include "Tools.h"

#include <cmath>
#include "String.h"
#include "DoubleList.h"
#include "SourceFile.h"
#include "Debug.h"

//TODO: split


void SizeLItoString(LARGE_INTEGER&liSize, WCHAR* wsSize)
{
	//calculating the size in KB, MB or GB.
	byte bWhich = 0;//0 = bytes; 1 = KB; 2 = MB; 3 = GB;
	double dbSize;

	{
		double dbKB, dbMB, dbGB;
		dbSize = (double)liSize.QuadPart;
		//check dSize in KB
		dbKB = liSize.QuadPart / (double)1024;
		if (dbKB > 1)
		{
			//we have size in dKB
			dbSize = dbKB;
			bWhich = 1;
			//check size in MB
			dbMB = dbKB / 1024;
			if (dbMB > 1)
			{
				//we have size in MB
				dbSize = dbMB;
				bWhich = 2;
				//check size in GB
				dbGB = dbMB / 1024;
				if (dbGB > 1) {dbSize = dbGB; bWhich = 3;}//we have size in GB
			}
		}
	}

	dbSize *= 100;
	dbSize = ceil(dbSize);
	dbSize /= 100;

	switch (bWhich)
	{
		//bytes
	case 0: StringFormatW(wsSize, L"%.2f bytes", dbSize); break;
		//KB
	case 1: StringFormatW(wsSize, L"%.2f KB", dbSize); break;
		//MB
	case 2: StringFormatW(wsSize, L"%.2f MB", dbSize); break;
		//GB
	case 3: StringFormatW(wsSize, L"%.2f GB", dbSize); break;
	}
}

int CountDigits(int nInteger)
{
	int size = 0;
	
	do
	{
		nInteger /= 10;
		size++;
	}while (nInteger);

	return size;
}

void SearchFolder(IShellFolder* pSearchFolder, CDoubleList<FILE_ITEM> &Items, LARGE_INTEGER& liSize)
{
	//getting the enumerator object to enumerate the items of the search folder
	IEnumIDList* pEnumIDList = NULL;
	HRESULT hr = pSearchFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &pEnumIDList);
	if (FAILED(hr))
	{
		if (hr != E_ACCESSDENIED)
			DisplayError(hr);
		return;
	}

	if (hr == S_FALSE) return;

	//getting pidl to each child item
	ITEMIDLIST* pidlChild = NULL;
	HRESULT hrEnum;
	do
	{
		hrEnum = pEnumIDList->Next(1, &pidlChild, NULL);
		if (FAILED(hrEnum))
		{
			pEnumIDList->Release();

			_ASSERT(0);
			DisplayError(hrEnum);
			return;
		}

		if (S_FALSE == hrEnum) break;

		//we need to know whether this is a folder or a file, and if it is a system item
		ULONG ulFlags = 0xFFFFFFFF;
		hr = pSearchFolder->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlChild, &ulFlags);
		if (FAILED(hr))
		{
			CoTaskMemFree(pidlChild);
			pidlChild = NULL;
			pEnumIDList->Release();

			_ASSERT(0);
			MessageBox(0, L"Could not get the attributes of the item: pSearchFolder->GetAttributesOf", 0, 0);
			return;
		}

		if (ulFlags & SFGAO_FILESYSTEM)
		{
			if (ulFlags & SFGAO_FOLDER && ulFlags & SFGAO_FILESYSANCESTOR && ulFlags & SFGAO_STORAGE)
			{
				//we need to search it
				IShellFolder* pNewSearchFolder = NULL;
				hr = pSearchFolder->BindToObject(pidlChild, NULL, IID_IShellFolder, (void**)&pNewSearchFolder);
				if (FAILED(hr))
				{
					CoTaskMemFree(pidlChild);
					pidlChild = NULL;
					pEnumIDList->Release();

					_ASSERT(0);
					MessageBox(0, L"Could not bind to new folder: pSearchFolder->BindToObject", 0, 0);
					return;
				}

				//it is a folder!!
				//get its full name
				STRRET strret;
				pSearchFolder->GetDisplayNameOf(pidlChild, SHGDN_FORPARSING, &strret);
				WCHAR* wsFullName;
				StrRetToStrW(&strret, NULL, &wsFullName);

				FILE_ITEM item;
				item.size = 0;

				item.wsFullName = wsFullName;
				item.type = ItemType::Folder;
				Items.push_back(item);

				SearchFolder(pNewSearchFolder, Items, liSize);
				pNewSearchFolder->Release();
			}
			else if (ulFlags & SFGAO_STREAM)
			{
				//it is a file!!
				//get its full name
				STRRET strret;
				pSearchFolder->GetDisplayNameOf(pidlChild, SHGDN_FORPARSING, &strret);
				WCHAR* wsFullName;
				StrRetToStrW(&strret, NULL, &wsFullName);

				FILE_ITEM item;
				LARGE_INTEGER li;
				CalcFileSize(wsFullName, li);
				item.size = li.QuadPart;
				liSize.QuadPart += item.size;

				item.wsFullName = wsFullName;
				item.type = ItemType::File;
				Items.push_back(item);
			}
		}

		CoTaskMemFree(pidlChild);
		pidlChild = NULL;

	#pragma warning(suppress: 4127)
	}while (1);

	if (pidlChild)
		CoTaskMemFree(pidlChild);
	pEnumIDList->Release();
}

BOOL CalcFileSize(const WCHAR* wsPath, LARGE_INTEGER& liSize)
{
	SourceFile File;
	if (FALSE == File.Open(wsPath)) return false;

	liSize.QuadPart = File.GetSize();
	File.Close();

	return true;
}

BOOL PathFileExistsEx(const WCHAR* wsPath)
{
	DWORD dwAttr = GetFileAttributes(wsPath);
	if (dwAttr == INVALID_FILE_ATTRIBUTES) return false;

	return true;
}

char* StringWtoA(const WCHAR* wstr)
{
	int len = StringLenW(wstr);
	len++;

	char* str = new char[len];
	for (int i = 0; i < len; i++)
	{
		*(str + i) = LOBYTE(*(wstr + i));
	}
	
	return str;
}

#define TIME_ONEDAY		86400 //60 * 60 * 24
#define TIME_ONEHOUR	3600 //60 * 60
#define TIME_ONEMINUTE	60

void FormatTime(WCHAR wsTime[20], DWORD dTime)
{
	WCHAR wsAux[10];
	memset(wsTime, 0, 30);

	//checking in days
	if (dTime >= TIME_ONEDAY) //more than a day
	{
		StringFormat(wsAux, L"%dd ", dTime / TIME_ONEDAY);
		StringCopyW(wsTime, wsAux);
		dTime = dTime % TIME_ONEDAY;
	}

	//checking in hours
	if (dTime >= TIME_ONEHOUR) //more than an hour
	{
		StringFormat(wsAux, L"%dh ", dTime / TIME_ONEHOUR);
		StringCatW(wsTime, wsAux);
		dTime = dTime % TIME_ONEHOUR;
	}

	//checking in minutes
	if (dTime >= TIME_ONEMINUTE) //more than a minute
	{
		StringFormat(wsAux, L"%dm ", dTime / TIME_ONEMINUTE);
		StringCatW(wsTime, wsAux);
		dTime = dTime % TIME_ONEMINUTE;
	}

	//checking in seconds
	if (dTime >= 0) //at least 0 seconds
	{
		StringFormat(wsAux, L"%ds ", dTime);
		StringCatW(wsTime, wsAux);
	}
}

void SpeedFtoString(float fSpeed, WCHAR* wsSpeed)
{
	//calculating the speed in KB/s, MB/s or GB/s.
	byte bWhich = 0;//0 = bytes; 1 = KB; 2 = MB; 3 = GB;
	{
		float fKB, fMB, fGB;
		//check fSpeed in KB/s
		fKB = fSpeed / (float)1024;
		if (fKB > 1)
		{
			//we have speed in fKB/s
			fSpeed = fKB;
			bWhich = 1;
			//check speed in MB/s
			fMB = fKB / 1024;
			if (fMB > 1)
			{
				//we have speed in MB/s
				fSpeed = fMB;
				bWhich = 2;
				//check speed in GB/s
				fGB = fMB / 1024;
				if (fGB > 1) {fSpeed = fGB; bWhich = 3;}//we have speed in GB/s
			}
		}
	}

	switch (bWhich)
	{
		//bytes/s
	case 0: StringFormat(wsSpeed, L"%.2f bytes/s", fSpeed); break;
		//KB/s
	case 1: StringFormat(wsSpeed, L"%.2f KB/s", fSpeed); break;
		//MB
	case 2: StringFormat(wsSpeed, L"%.2f MB/s", fSpeed); break;
		//GB
	case 3: StringFormat(wsSpeed, L"%.2f GB/s", fSpeed); break;
	}
}

void __stdcall OnDestroyFileItemCoTask(FILE_ITEM& fitem)
{
	CoTaskMemFree(fitem.wsFullName);
}

