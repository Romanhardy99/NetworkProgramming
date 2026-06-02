#include"FormatLastError.h"

CHAR* FormatLastError(DWORD dwError, CHAR szError[])
{
	LPSTR lpError = NULL;
	FormatMessage
	(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpError,
		0,
		NULL
	);
	//strcpy(szError, lpError);
	sprintf(szError, "Error %i: %s", dwError, lpError);
	LocalFree(lpError);
	return szError;
}