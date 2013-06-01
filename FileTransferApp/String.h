#pragma warning (disable: 4995 4996)

#define StringCopy wcscpy
#define StringCopyW wcscpy
#define StringCopyA strcpy

#define StringCat wcscat
#define StringCatW wcscat
#define StringCatA strcat

#define StringFormat swprintf
#define StringFormatW swprintf
#define StringFormatA sprintf

#define StringComp wcscmp
#define StringCompW wcscmp
#define StringCompA strcmp

#define StringIComp wcsicmp
#define StringICompW wcsicmp
#define StringICompA stricmp

#define StringLen wcslen
#define StringLenW wcslen
#define StringLenA strlen

#define StringStr wcsstr
#define StringStrW wcsstr
#define StringStrA strstr

#define StringChar wcschr
#define StringCharW wcschr
#define StringCharA strchr

#define StringRevChar wcsrchr
#define StringRevCharW wcsrchr
#define StringRevCharA strrchr

#define StringUpper wcsupr
#define StringUpperW wcsupr
#define StringUpperA strupr

#define StringLower wsclwr
#define StringLowerW wsclwr
#define StringLowerA strlwr