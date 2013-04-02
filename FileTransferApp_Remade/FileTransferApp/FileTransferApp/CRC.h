#include "General.h"

typedef DWORD				CRC;
#define INITIAL_REMAINDER	0xFFFFFFFF
#define FINAL_XOR_VALUE		0xFFFFFFFF
#define POLYNOMIAL			0x04C11DB7

#define WIDTH    (8 * sizeof(CRC))
#define TOPBIT   (1 << (WIDTH - 1))

static DWORD reflect(DWORD data, BYTE nBits);

#define REFLECT_DATA(X)			((BYTE) reflect((X), 8))
#define REFLECT_REMAINDER(X)	((CRC) reflect((X), WIDTH))

void crcInit(void);
CRC CRCCalc(BYTE const message[], int nBytes);