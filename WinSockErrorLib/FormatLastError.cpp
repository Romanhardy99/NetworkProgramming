#include "FormatLastError.h"
#include <Windows.h>

std::string FormatLastError(int errorCode)
{
    LPSTR buffer = nullptr;

    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buffer,
        0,
        NULL);

    std::string message;

    if (size)
    {
        message = buffer;
        LocalFree(buffer);
    }
    else
    {
        message = "Unknown error";
    }

    return message;
}