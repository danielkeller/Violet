#ifndef WINDOWS_HPP
#define WINDOWS_HPP

#undef APIENTRY
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

inline void ThrowErrno(const std::string& text)
{
	DWORD errcode = GetLastError();
	LPTSTR lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::string message = lpMsgBuf;
	LocalFree(lpMsgBuf);
	throw std::runtime_error(text + ": " + message);
}

using HandleRAII = std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&::CloseHandle)>;

#endif