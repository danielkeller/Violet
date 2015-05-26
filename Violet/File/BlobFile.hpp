#ifndef BLOB_FILE_HPP
#define BLOB_FILE_HPP

#include <fstream>
#include <array>
#include "Containers/WrappedIterator.hpp"

#include "Utils/ZipBuf.hpp"

struct BlobFileException : public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

using BlobSizeType = std::uint64_t;
using BlobMagicType = std::array<char, 4>;

//Very simple user-structured data file
class BlobOutFile
{
public:
	BlobOutFile(const std::string& path, const BlobMagicType& magic, std::uint32_t version);
	
	template<typename T>
	void Write(const T& val)
	{
		zip.sputn(reinterpret_cast<const char*>(&val), sizeof(val));
	}

	void Write(bool val);

	template<typename T>
	void Write(const std::vector<T>& val)
	{
		Write<BlobSizeType>(val.size());
		zip.sputn(reinterpret_cast<const char*>(val.data()), sizeof(T)*val.size());
	}

	template<typename T>
	void Write(range<const T*>& val)
	{
		Write<BlobSizeType>(val.size());
		zip.sputn(reinterpret_cast<const char*>(val.begin()), sizeof(T)*val.size());
	}

	void Write(const std::string& val);

private:
	std::filebuf file;
	zip_buf zip;
};

class BlobInFile
{
public:
	BlobInFile(const std::string& path, const BlobMagicType& magic, std::uint32_t version);

	template<typename T>
	T Read()
	{
		T ret;
		if (zip.sgetn(reinterpret_cast<char*>(&ret), sizeof(ret)) != sizeof(ret))
			ThrowEOF();
		return ret;
	}

	template<typename T, typename U>
	T Read()
	{
		return static_cast<T>(Read<U>());
	}

	bool ReadBool();

	template<typename T>
	std::vector<T> ReadVector()
	{
		auto size = static_cast<std::vector<T>::size_type>(Read<BlobSizeType>());
		std::vector<T> ret(size);
		if (zip.sgetn(reinterpret_cast<char*>(ret.data()), sizeof(T)*size) != sizeof(T)*size)
			ThrowEOF();
		return ret;
	}

	std::string ReadString();

private:
	std::filebuf file;
	zip_buf zip;

	void ThrowEOF() const;
};

#endif
