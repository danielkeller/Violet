#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include <iterator>
#include "Containers/WrappedIterator.hpp"

struct DirIterImpl;

class DirIter
{
public:
	DirIter() = default; //end
	DirIter(std::string path); //begin
	using iterator_category = std::input_iterator_tag;
	using value_type = std::string;
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

#ifdef _WIN32
#else //todo: get rid of this
	~MappedFile();
#endif

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
	void* ptr;
	int fd; //TODO: this isn't actually needed
#endif

	void ThrowErrno(const std::string& text);
};

//Note that this is subject to a race condition: if the file is deleted in between
//when we check for it and open it. This could be fixed but that seems like a lot of work
bool CacheIsFresh(const std::string& file, const std::string& cache);

#endif