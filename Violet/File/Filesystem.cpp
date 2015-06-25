#include "stdafx.h"

#include "Filesystem.hpp"

//common

range<DirIter> Browse(std::string dir)
{
    return{ { dir }, {} };
}

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

//need the return in case exceptions are off
#define MAP_FILE_ERR(str) do {ThrowErrno(str); return;} while(false)

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
	: dothrow(true), length(0), ptr(nullptr, &::UnmapViewOfFile)
{}

void MappedFile::ThrowErrno(const std::string& text)
{
	if (dothrow)
		ThrowErrno(text);
}

#define CHECKED_CLOSE(ptr, str) do { \
	if(!ptr.get_deleter()(ptr.release())) MAP_FILE_ERR(str);} while(false)

void MappedFile::Open(const std::string& name)
{
	EXCEPT_INFO_BEGIN

	ptr.reset();

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

bool CacheIsFresh(const std::string& file, const std::string& cache)
{
	WIN32_FIND_DATA ffd;
	FILETIME cacheTime, fileTime;

	HandleRAII hFindCache(::FindFirstFile(cache.c_str(), &ffd), &::FindClose);

	if (hFindCache.get() == INVALID_HANDLE_VALUE)
	{
		if (::GetLastError() == ERROR_FILE_NOT_FOUND)
			return false;
		else
			ThrowErrno("FindFirstFile");
	}

	cacheTime = ffd.ftLastWriteTime;

	HandleRAII hFindFile(::FindFirstFile(file.c_str(), &ffd), &::FindClose);

	if (hFindFile.get() == INVALID_HANDLE_VALUE)
	{
		if (::GetLastError() == ERROR_FILE_NOT_FOUND)
			return true; //so we can delete the original
		else
			ThrowErrno("FindFirstFile");
	}

	fileTime = ffd.ftLastWriteTime;

	return std::tie(cacheTime.dwHighDateTime, cacheTime.dwLowDateTime)
		> std::tie(fileTime.dwHighDateTime, fileTime.dwLowDateTime);
}

#else

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <dirent.h>

void ThrowErrno(const std::string& text)
{
    throw std::runtime_error(text + ": " + strerror(errno));
}

struct DirIterImpl
{
    DirIterImpl() : dir(nullptr, &closedir) {}
    std::string dirName;
    struct dirent entry;
    std::unique_ptr<DIR, decltype(&closedir)> dir;
    
    bool CheckedClose() {return !dir || closedir(dir.release()) != 0;}
};

DirIter::DirIter(std::string path) //begin
: impl(std::make_shared<DirIterImpl>())
{
    DIR* dir = opendir(path.c_str());
    if (!dir) ThrowErrno(path);
    impl->dir.reset(dir);
    
    operator++();
    
    impl->dirName = path;
}

std::string DirIter::operator*()
{
    return impl->dirName + '/' + impl->entry.d_name;
}

DirIter& DirIter::operator++()
{
    struct dirent* result;
    if (readdir_r(impl->dir.get(), &impl->entry, &result) != 0)
        throw std::runtime_error(impl->dirName + ": invalid directory");
    
    if (!result)
    {
        if (impl->CheckedClose())
            ThrowErrno("closedir");
        impl.reset();
    }
    
    return *this;
}

MappedFile::MappedFile()
	: dothrow(true), ptr(nullptr), length(0)
{}

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

void MappedFile::ThrowErrno(const std::string& text)
{
    if (dothrow) ::ThrowErrno(text);
}

class FdRAII
{
public:
    FdRAII(int fd) : fd(fd) {}
    int get() {return fd;}
    explicit operator bool() {return fd != -1;}
    bool CheckedClose()
    {
        int toClose = -1; std::swap(toClose, fd); //clear fd first
        return toClose == -1 || close(toClose) != -1;
    }
    ~FdRAII() {CheckedClose();}
private:
    int fd;
};

void MappedFile::Open(const std::string& name)
{
    EXCEPT_INFO_BEGIN
    
	Close();

	FdRAII fd = open(name.c_str(), O_RDONLY);
    if (!fd) MAP_FILE_ERR("open");

	struct stat sb;
	if (fstat(fd.get(), &sb) == -1)
        MAP_FILE_ERR("fstat");

	length = sb.st_size;
	void* mapptr = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd.get(), 0);
	if (mapptr == MAP_FAILED)
        MAP_FILE_ERR("mmap");
    
    ptr.reset(mapptr, [this](void* p)
    {
        return munmap(p, length);
    });
    
    EXCEPT_INFO_END("MappedFile " + name)
}

void MappedFile::Close()
{
	if (!*this)
		return;
    
	auto ptr1 = ptr.get();
	ptr = nullptr;

	if (munmap(ptr1, length) == -1)
		ThrowErrno("munmap");
}

bool CacheIsFresh(const std::string& file, const std::string& cache)
{
    struct stat fileStat, cacheStat;
    
    if (stat(cache.c_str(), &cacheStat) == -1)
    {
        if (errno == ENOENT) //no cache
            return false;
        else
            ThrowErrno(cache);
    }
    
    if (stat(file.c_str(), &fileStat) == -1)
    {
        if (errno == ENOENT) //no original (this is ok)
            return true;
        else
            ThrowErrno(file);
    }
    
    return cacheStat.st_mtime > fileStat.st_mtime;
}

#endif
