#ifndef MAPPED_FILE_HPP
#define MAPPED_FILE_HPP

#include <string>

#ifdef _WIN32
#include <Windows.h>
#endif

class MappedFile
{
public:
    MappedFile()
        : isopen(false), dothrow(false), ptr(nullptr), length(0)
    {}

    MappedFile(const std::string& name)
    {
        Open(name);
    }

    MappedFile(const MappedFile&) = delete;
    MappedFile(MappedFile&&);
    MappedFile& operator=(MappedFile f);

    ~MappedFile()
    {
		dothrow = false; //Close can throw
        Close();
    }

    void Throws(bool dothrow_)
    {
        dothrow = dothrow_;
    }

    void Open(const std::string& name);
    void Close();

    bool operator!()
    {
        return !isopen;
    }
    explicit operator bool()
    {
        return !!*this;
    }

    template<class T>
    const T* Data()
    {
        return static_cast<T*>(ptr);
    }
    size_t Size()
    {
        return length;
    }

private:
    bool isopen;
    bool dothrow;
    void* ptr;
    size_t length;
#ifdef _WIN32
#else
    int fd; //TODO: this isn't actually needed
#endif

    void ThrowErrno(const std::string& text);
};

#endif