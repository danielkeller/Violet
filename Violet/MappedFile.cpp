#include "MappedFile.hpp"
#include <tuple>

MappedFile& MappedFile::operator=(MappedFile f)
{
	auto other = std::tie(f.isopen, f.dothrow, f.ptr, f.length);
	std::tie(isopen, dothrow, ptr, length).swap(other);
#ifdef _WIN32
#else
	std::swap(fd, f.fd);
#endif
    return *this;
}

MappedFile::MappedFile(MappedFile&& f)
    : isopen(f.isopen), dothrow(f.dothrow), ptr(f.ptr), length(f.length)
#ifdef _WIN32
#else
    ,fd(f.fd)
#endif
{
    f.isopen = false;
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

struct HandleRAII
{
	HANDLE h;
	HandleRAII(HANDLE h_) : h(h_) {}
	HandleRAII(const HandleRAII&) = delete;
	~HandleRAII() { if (h) CloseHandle(h); }
	bool CheckedClose()
	{
		bool result = CloseHandle(h) != 0;
		h = nullptr;
		return result;
	}
};

void MappedFile::Open(const std::string& name)
{
	isopen = false;
	//TODO: This prevents other processes from modifying the file
	HandleRAII fileHandle{ CreateFile(name.c_str(), GENERIC_READ, 0, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr) };
	if (fileHandle.h == INVALID_HANDLE_VALUE)
	{
		ThrowErrno("CreateFile");
		return;
	}

	HandleRAII fileMapping{ CreateFileMapping(
		fileHandle.h, nullptr, PAGE_READONLY, 0, 0, nullptr) };
	if (fileMapping.h == nullptr) //Windows is so consistent
	{
		ThrowErrno("CreateFileMapping");
		return;
	}

	if (!fileHandle.CheckedClose())
	{
		ThrowErrno("CloseHandle(fileHandle)");
		return;
	}

	ptr = MapViewOfFile(fileMapping.h, FILE_MAP_READ, 0, 0, 0);
	if (ptr == nullptr)
	{
		ThrowErrno("MapViewOfFile");
		return;
	}

	if (!fileMapping.CheckedClose())
	{
		auto err = GetLastError();
		UnmapViewOfFile(ptr);
		SetLastError(err);
		ThrowErrno("CloseHandle(fileHandle)");
		return;
	}

	MEMORY_BASIC_INFORMATION memInfo;
	if (VirtualQuery(ptr, &memInfo, sizeof(memInfo)) == 0)
	{
		auto err = GetLastError();
		UnmapViewOfFile(ptr);
		SetLastError(err);
		ThrowErrno("VirtualQuery");
		return;
	}
	length = memInfo.RegionSize;
	isopen = true;
}

void MappedFile::Close()
{
	if (!isopen)
		return;
	isopen = false;

	if (UnmapViewOfFile(ptr) == 0)
		ThrowErrno("UnmapViewOfFile");
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
    isopen = false;

    if (close(fd) == -1)
    {
        int temp = errno;
        munmap(ptr, length);
        errno = temp;
        ThrowErrno("close");
    }
    if (munmap(ptr, length) == -1)
    {
        ThrowErrno("munmap");
    }
}
#endif
