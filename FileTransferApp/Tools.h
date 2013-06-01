#pragma once

#include "General.h"

//TODO: split

//converts a size from LARGE_INTEGER to a user-friendly string.
void SizeLItoString(LARGE_INTEGER&liSize, WCHAR* wsSize);

//counts the digits of the specified integer number
int CountDigits(int nInteger);

//searches within the folder: all file information is stored in Items; total size is stored in liSize.
void SearchFolder(IShellFolder* pSearchFolder, CDoubleList<FILE_ITEM> &Items, LARGE_INTEGER& liSize);

//calculates the size of the specified file
BOOL CalcFileSize(const WCHAR* wsPath, LARGE_INTEGER& liSize);
//checks to see if the specified file/folder exists or not.
BOOL PathFileExistsEx(const WCHAR* wsPath);

//converts a wide string into an ansi string
char* StringWtoA(WCHAR* wstr);

//formats the time in hh:mm:ss format
void FormatTime(WCHAR wsTime[20], DWORD dTime);

//converts a double that represents the transfer speed into a user-friendly transfer speed.
void SpeedFtoString(float fSpeed, WCHAR* wsSpeed);

//called by the doublelist when an item must be removed
void __stdcall OnDestroyFileItemCoTask(FILE_ITEM& fitem);