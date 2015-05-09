#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include <memory>
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

#endif