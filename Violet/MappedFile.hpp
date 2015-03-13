#ifndef MAPPED_FILE_HPP
#define MAPPED_FILE_HPP

#include <string>

#ifdef _WIN32
#include <memory>
#endif

class MappedFile
{
public:
#ifdef _WIN32
	MappedFile();
#else
	MappedFile()
		: dothrow(false), ptr(nullptr), length(0)
	{}
#endif

    MappedFile(const std::string& name)
		: MappedFile()
    {
        Open(name);
    }

    MappedFile(const MappedFile&) = delete;
    MappedFile(MappedFile&&);
    MappedFile& operator=(MappedFile f);

#ifdef _WIN32
#else //todo: get rid of this
	~MappedFile()
	{
		dothrow = false; //Close can throw
		Close();
	}
#endif

    void Throws(bool dothrow_)
    {
        dothrow = dothrow_;
    }

    void Open(const std::string& name);
    void Close();

    bool operator!()
    {
        return !ptr;
    }
    explicit operator bool()
    {
        return !!*this;
    }

    template<class T>
    const T* Data()
    {
        return static_cast<T*>(ptr.get());
    }
    size_t Size()
    {
        return length;
    }

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

#endif
