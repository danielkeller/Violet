#ifndef BLOB_FILE_HPP
#define BLOB_FILE_HPP

#include <fstream>
#include <array>
#include "Containers/WrappedIterator.hpp"

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
		str.write(reinterpret_cast<const char*>(&val), sizeof(val));
	}

	void Write(bool val);

	template<typename T, typename Alloc>
	void Write(const std::vector<T, Alloc>& val)
	{
		Write<BlobSizeType>(val.size());
		str.write(reinterpret_cast<const char*>(val.data()), sizeof(T)*val.size());
	}

	template<typename T>
	void Write(range<const T*>& val)
	{
		Write<BlobSizeType>(val.size());
		str.write(reinterpret_cast<const char*>(val.begin()), sizeof(T)*val.size());
	}

	void Write(const std::string& val);

private:
	std::ofstream str;
};

class BlobInFile
{
public:
	BlobInFile(const std::string& path, const BlobMagicType& magic, std::uint32_t version);

	template<typename T>
	T Read()
	{
		T ret;
		str.read(reinterpret_cast<char*>(&ret), sizeof(ret));
		CheckAndThrow();
		return ret;
	}

	template<typename T, typename U>
	T Read()
	{
		U ret;
		str.read(reinterpret_cast<char*>(&ret), sizeof(ret));
		CheckAndThrow();
		return static_cast<T>(ret);
	}

	bool ReadBool();

	template<typename T, typename Alloc = std::allocator<T>>
	std::vector<T, Alloc> ReadVector()
	{
		auto size = static_cast<std::vector<T, Alloc>::size_type>(Read<BlobSizeType>());
		std::vector<T, Alloc> ret(size);
		str.read(reinterpret_cast<char*>(ret.data()), sizeof(T)*size);
		CheckAndThrow();
		return ret;
	}

	std::string ReadString();

private:
	std::ifstream str;

	void CheckAndThrow() const;
};

#endif
