#ifndef ZIP_BUF_HPP
#define ZIP_BUF_HPP

#include <sstream>
#include <memory>

class zip_buf : public std::stringbuf
{
	using super = std::stringbuf;
public:
	zip_buf();
	zip_buf(std::streambuf* underlying);
	~zip_buf();

	void underlying(std::streambuf*);
	std::streambuf* underlying();

protected:
	int_type overflow(int_type c) override;
	int_type underflow() override;
	int sync() override;

private:
	static const size_t BLOCK_SIZE = 1024;
	int_type write_some(std::streamsize min);

	std::streambuf* stream;
};

#endif
