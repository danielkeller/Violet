#include "stdafx.h"
#include "ZipBuf.hpp"

extern "C" {
#include "lzf/lzf.h"
}

zip_buf::zip_buf()
	: stream(nullptr)
{}

zip_buf::zip_buf(std::streambuf* underlying)
	: stream(underlying)
{}

zip_buf::~zip_buf()
{
	sync();
}

void zip_buf::underlying(std::streambuf* underlying)
{
	stream = underlying;
}

std::streambuf* zip_buf::underlying()
{
	return stream;
}

#include <iostream>

zip_buf::int_type zip_buf::overflow(int_type c)
{
	if (write_some(BLOCK_SIZE) == traits_type::eof())
		return traits_type::eof();
	return super::overflow(c);
}

int zip_buf::sync()
{
	while (pptr() - pbase() > 0)
		if (write_some(0) == traits_type::eof())
			return -1;
	if (stream) return stream->pubsync();
	return 0;
}

zip_buf::int_type zip_buf::write_some(std::streamsize min)
{
	auto uncompSize = static_cast<std::uint16_t>(
		std::min(size_t(pptr() - pbase()), BLOCK_SIZE));

	if (stream && uncompSize >= min)
	{
		char buf[BLOCK_SIZE];

		std::uint16_t compSize =
			lzf_compress(pbase(), uncompSize, buf, BLOCK_SIZE);

		if (stream->sputn(reinterpret_cast<const char*>(&uncompSize), sizeof uncompSize)
			!= sizeof uncompSize)
			return traits_type::eof();
		if (stream->sputn(reinterpret_cast<const char*>(&compSize), sizeof compSize)
			!= sizeof compSize)
			return traits_type::eof();

		if (compSize != 0) //did compress
		{
			if (stream->sputn(buf, compSize) != compSize)
				return traits_type::eof();
		}
		else //did not compress
		{
			if (stream->sputn(pbase(), uncompSize) != uncompSize)
				return traits_type::eof();
		}

		//dump the characters from the buffer
		str(str().substr(uncompSize));
	}

	return 0;
}

zip_buf::int_type zip_buf::underflow()
{
	int_type ret = super::underflow();
	if (ret != traits_type::eof())
		return ret;

	std::uint16_t compSize, uncompSize;
	if (stream->sgetn(reinterpret_cast<char*>(&uncompSize), sizeof uncompSize)
		!= sizeof uncompSize)
		return traits_type::eof(); //stream probably hit eof
	if (stream->sgetn(reinterpret_cast<char*>(&compSize), sizeof compSize)
		!= sizeof compSize)
		return traits_type::eof(); //stream probably hit eof

	if (compSize > BLOCK_SIZE || uncompSize > BLOCK_SIZE) //not reading the right thing
		return traits_type::eof();

	char buf[BLOCK_SIZE];

	if (compSize == 0) //uncompressed
		stream->sgetn(buf, uncompSize);
	else
	{
		char comp[BLOCK_SIZE];
		stream->sgetn(comp, compSize);
		if (lzf_decompress(comp, compSize, buf, uncompSize) != uncompSize)
			return traits_type::eof(); //decompressed the wrong amount of stuff
	}

	str({ buf, buf + uncompSize });

	return super::underflow();
}
