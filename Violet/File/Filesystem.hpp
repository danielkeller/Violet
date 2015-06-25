#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include <iterator>
#include "Containers/WrappedIterator.hpp"

struct DirIterImpl;

class DirIter : public std::iterator<std::input_iterator_tag, std::string>
{
public:
	DirIter() = default; //end
	DirIter(std::string path); //begin
	BASIC_EQUALITY(DirIter, impl);

	std::string operator*();
	DirIter& operator++();
private:
	std::shared_ptr<DirIterImpl> impl;
};

range<DirIter> Browse(std::string dir);

class MappedFile
{
public:
	MappedFile();
	MappedFile(const std::string& name);
	MappedFile(const MappedFile&) = delete;
	MappedFile(MappedFile&&);
	MappedFile& operator=(MappedFile f);

	void Throws(bool dothrow_);

	void Open(const std::string& name);
	void Close();

	bool operator!();
	explicit operator bool();

	template<class T>
	const T* Data()
    {
		return static_cast<T*>(ptr.get());
	}
	size_t Size();

private:
	bool dothrow;
	size_t length;
#ifdef _WIN32
	typedef int __stdcall deleter(const void*);
	std::unique_ptr<void, deleter*> ptr;
#else
    std::shared_ptr<void> ptr;
#endif

	void ThrowErrno(const std::string& text);
};

//Note that this is subject to a race condition: if the file is deleted in between
//when we check for it and open it. This could be fixed but that seems like a lot of work
bool CacheIsFresh(const std::string& file, const std::string& cache);

#endif