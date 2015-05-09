#include "stdafx.h"

#include "Filesystem.hpp"

//Todo: posix
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
	: impl(std::make_unique<DirIterImpl>())
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