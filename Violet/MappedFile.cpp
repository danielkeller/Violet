#include "stdafx.h"
#include "MappedFile.hpp"
#include <tuple>

MappedFile& MappedFile::operator=(MappedFile f)
{
	auto other = std::tie(f.dothrow, f.ptr, f.length);
	std::tie(dothrow, ptr, length).swap(other);
#ifdef _WIN32
#else
	std::swap(fd, f.fd);
#endif
    return *this;
}

MappedFile::MappedFile(MappedFile&& f)
    : dothrow(f.dothrow), ptr(std::move(f.ptr)), length(f.length)
#ifdef _WIN32
#else
    ,fd(f.fd)
#endif
{
}

#ifdef _WIN32

void MappedFile::ThrowErrno(const std::string& text)
{
	if (dothrow)
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
}

//need the return in case exceptions are off
#define MAP_FILE_ERR(str) do {ThrowErrno(str); return;} while(false)
#define CHECKED_CLOSE(ptr, str) do { \
	if(!ptr.get_deleter()(ptr.release())) MAP_FILE_ERR(str);} while(false)

void MappedFile::Open(const std::string& name)
{
	EXCEPT_INFO_BEGIN

	ptr.reset();

	using HandleRAII = std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&::CloseHandle)>;

	//TODO: This prevents other processes from modifying the file
	HandleRAII fileHandle(CreateFile(name.c_str(), GENERIC_READ, 0, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr), &::CloseHandle);

	if (fileHandle.get() == INVALID_HANDLE_VALUE)
		MAP_FILE_ERR("CreateFile");

	HandleRAII fileMapping{ CreateFileMapping(
		fileHandle.get(), nullptr, PAGE_READONLY, 0, 0, nullptr), &::CloseHandle };

	if (fileMapping.get() == nullptr) //Windows is so consistent
		MAP_FILE_ERR("CreateFileMapping");

	CHECKED_CLOSE(fileHandle, "CloseHandle(fileHandle)");
	
	std::unique_ptr<void, decltype(&::UnmapViewOfFile)> ptr1(
		MapViewOfFile(fileMapping.get(), FILE_MAP_READ, 0, 0, 0), &::UnmapViewOfFile);

	if (ptr1 == nullptr)
		MAP_FILE_ERR("MapViewOfFile");

	CHECKED_CLOSE(fileMapping, "CloseHandle(fileMapping)");

	MEMORY_BASIC_INFORMATION memInfo;
	if (VirtualQuery(ptr1.get(), &memInfo, sizeof(memInfo)) == 0)
		MAP_FILE_ERR("VirtualQuery");

	length = memInfo.RegionSize;

	//set the pointer, making the MappedFile open
	ptr = std::move(ptr1);

	EXCEPT_INFO_END("MappedFile " + name)
}

void MappedFile::Close()
{
	if (!*this)
		return;
	
	CHECKED_CLOSE(ptr, "UnmapViewOfFile");
}

#else

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

void MappedFile::ThrowErrno(const std::string& text)
{
    if (dothrow)
        throw std::runtime_error(text + ": " + strerror(errno));
}

void MappedFile::Open(const std::string& name)
{
	Close();

    fd = open(name.c_str(), O_RDONLY);
    if (fd == -1)
    {
        ThrowErrno("open");
        return;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        int temp = errno;
        close(fd);
        errno = temp;
        ThrowErrno("fstat");
        return;
    }

    length = sb.st_size;
    ptr = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ptr == MAP_FAILED)
    {
        int temp = errno;
        close(fd);
        errno = temp;
        ThrowErrno("mmap");
        return;
    }
    isopen = true;
}

void MappedFile::Close()
{
    if (!isopen)
        return;
	auto ptr1 = ptr;
	ptr = nullptr;

    if (close(fd) == -1)
    {
        int temp = errno;
		munmap(ptr1, length);
        errno = temp;
        ThrowErrno("close");
    }
	if (munmap(ptr1, length) == -1)
    {
        ThrowErrno("munmap");
    }
}
#endif
