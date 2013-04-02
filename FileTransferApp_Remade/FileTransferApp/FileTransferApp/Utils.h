#include "General.h"

struct ToUpper
{
	void operator()(char& ch)
	{
		ch = toupper(ch);
	}
};