#include "stdafx.h"

#include "Filesystem.hpp"

//common
MappedFile::MappedFile(const std::string& name)
	: MappedFile()
{
	Open(name);
}

void MappedFile::Throws(bool dothrow_)
{
	dothrow = dothrow_;
}

bool MappedFile::operator!()
{
	return !ptr;
}

MappedFile::operator bool()
{
	return !!*this;
}

size_t MappedFile::Size()
{
	return length;
}

#ifdef _WIN32
#include "Windows.hpp"

struct DirIterImpl
{
	DirIterImpl() : hFind(0) {}
	~DirIterImpl() { ::FindClose(hFind); }
	std::string dir;
	WIN32_FIND_DATA ffd;
	HANDLE hFind;
};

DirIter::DirIter(std::string path) //begin
	: impl(std::make_shared<DirIterImpl>())
{
	if (path.size() + 3 > MAX_PATH) //3 is for "/*\0"
		throw std::domain_error("Path is too long");

	impl->hFind = ::FindFirstFile((path + "/*").c_str(), &impl->ffd);

	if (impl->hFind == INVALID_HANDLE_VALUE)
	{
		if (::GetLastError() == ERROR_FILE_NOT_FOUND)
			impl.reset();
		else
			ThrowErrno("DirIter");
	}

	impl->dir = path;
}

std::string DirIter::operator*()
{
	return impl->dir + '/' + impl->ffd.cFileName;
}

DirIter& DirIter::operator++()
{
	if (!::FindNextFile(impl->hFind, &impl->ffd))
	{
		if (::GetLastError() == ERROR_NO_MORE_FILES)
			impl.reset();
		else
			ThrowErrno("DirIter");
	}
	return *this;
}

range<DirIter> Browse(std::string dir)
{
	return{ { dir }, {} };
}

MappedFile& MappedFile::operator=(MappedFile f)
{
	auto other = std::tie(f.dothrow, f.ptr, f.length);
	std::tie(dothrow, ptr, length).swap(other);
	return *this;
}

MappedFile::MappedFile(MappedFile&& f)
	: dothrow(f.dothrow), length(f.length), ptr(std::move(f.ptr))
{
}

MappedFile::MappedFile()
	: dothrow(false), length(0), ptr(nullptr, &::UnmapViewOfFile)
{}

void MappedFile::ThrowErrno(const std::string& text)
{
	if (dothrow)
		ThrowErrno(text);
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

MappedFile::MappedFile()
	: dothrow(false), ptr(nullptr), length(0)
{}

MappedFile::~MappedFile()
{
	dothrow = false; //Close can throw
	Close();
}

MappedFile& MappedFile::operator=(MappedFile f)
{
	auto other = std::tie(f.dothrow, f.ptr, f.length, f.fd);
	std::tie(dothrow, ptr, length, fd).swap(other);
	return *this;
}

MappedFile::MappedFile(MappedFile&& f)
	: dothrow(f.dothrow), length(f.length), ptr(std::move(f.ptr))
	, fd(f.fd)
{
}

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
