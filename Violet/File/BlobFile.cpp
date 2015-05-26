#include "stdafx.h"
#include "BlobFile.hpp"

static const std::uint16_t endianTest = 0xFFAA;

BlobOutFile::BlobOutFile(const std::string& path
	, const BlobMagicType& magic, std::uint32_t version)
	: zip(&file)
{
	if (!file.open(path, std::ofstream::binary | std::ofstream::trunc | std::ofstream::out))
		throw std::runtime_error("Could not open file '" + path + '\'');

	Write(endianTest);
	Write(magic);
	Write(version);
}

void BlobOutFile::Write(const std::string& val)
{
	Write<BlobSizeType>(val.size());
	zip.sputn(val.data(), val.size());
}

void BlobOutFile::Write(bool val)
{
	Write<std::uint8_t>(val ? 1 : 0);
}

BlobInFile::BlobInFile(const std::string& path, const BlobMagicType& magic, std::uint32_t version)
	: zip(&file)
{
	if (!file.open(path, std::ofstream::binary | std::ofstream::in))
		throw BlobFileException("Could not open file '" + path + '\'');

	if (endianTest != Read<std::uint16_t>())
		throw BlobFileException("Incorrect endianness in file '" + path + '\'');

	if (magic != Read<BlobMagicType>())
		throw BlobFileException("Incorrect type of file '" + path + '\'');

	if (version != Read<std::uint32_t>())
		throw BlobFileException("Incorrect version of file '" + path + '\'');
}

bool BlobInFile::ReadBool()
{
	return Read<std::uint8_t>() == 1;
}

std::string BlobInFile::ReadString()
{
	auto vec = ReadVector<char>();
	return{ vec.begin(), vec.end() };
}

void BlobInFile::ThrowEOF() const
{
	throw BlobFileException("Insufficient data in blob file");
}