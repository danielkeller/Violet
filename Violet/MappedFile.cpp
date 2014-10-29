#include "MappedFile.hpp"
#include <tuple>

MappedFile& MappedFile::operator=(MappedFile f)
{
#ifdef _WIN32
#else
    auto other = std::tie(f.isopen, f.dothrow, f.ptr, f.length, f.fd);
    std::tie(isopen, dothrow, ptr, length, fd).swap(other);
#endif
    return *this;
}

MappedFile::MappedFile(MappedFile&& f)
    : isopen(f.isopen), dothrow(f.dothrow), ptr(f.ptr), length(f.length),
#ifdef _WIN32
#else
    fd(f.fd)
#endif
{
    f.isopen = false;
}

#ifdef _WIN32
#else
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
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
    isopen = false;
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
