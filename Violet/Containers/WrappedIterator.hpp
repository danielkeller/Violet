#ifndef WRAPPED_ITERATOR_HPP
#define WRAPPED_ITERATOR_HPP

#include <cstddef>
#include <iterator>

//holds a pair of iterators, can be used in range-for
template<class Iter>
class range
{
public:
	using iterator = Iter;
	range(const range&) = default;
	range(iterator b, iterator e) : begin_(b), end_(e) {}
	range(iterator e) : begin_(e), end_(e) {} //empty range
	iterator begin() { return begin_; }
	iterator end() { return end_; }

	//Don't compile this if it doesn't make sense
	template<class ret = typename std::iterator_traits<iterator>::difference_type>
	ret size() const
	{
		return end_ - begin_;
	}

private:
	iterator begin_, end_;
};

template<class Iter>
range<Iter> make_range(Iter b, Iter e) { return{ b, e }; }

#endif
