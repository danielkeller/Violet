#ifndef WRAPPED_ITERATOR_HPP
#define WRAPPED_ITERATOR_HPP

#include <cstddef>

//Make the creation of custom iterators easier
//implementers just override whatever they want

template<class Derived, class IterTy, class ValueTy = typename IterTy::value_type>
class WrappedIterator
{
public:
	using value_type = ValueTy;
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = ptrdiff_t;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using reference = value_type&;
	using const_reference = value_type&;

	template<class dummy = void> //Don't compile this if it doesn't make sense
	reference operator*() { return *it; }
	const_reference operator*() const { return *DerivedThisUnconst(); }
	pointer operator->() { return &**DerivedThis(); }
	const_pointer operator->() const { return &**DerivedThisUnconst(); }
	
	Derived& operator++() { ++it; return *DerivedThis(); }
	Derived operator++(int) const { auto ret = *DerivedThis(); ++ret; return ret; }
	Derived& operator--() { --it; return *DerivedThis(); }
	Derived operator--(int) const { auto ret = *DerivedThis(); --ret; return ret; }
    //TODO: use std::advance
	Derived& operator+=(ptrdiff_t o) { it += o; return *DerivedThis(); }
	Derived operator+(ptrdiff_t o) const { auto ret(*DerivedThis()); ret.it = it + o; return ret; }
	Derived operator-(ptrdiff_t o) const { auto ret(*DerivedThis()); ret.it = it - o; return ret; }
	ptrdiff_t operator-(const Derived& other) const { return it - other.it; }
	
	bool operator==(const Derived& other) const { return it == other.it; }
	bool operator!=(const Derived& other) const { return it != other.it; }
	bool operator<(const Derived& other) const { return it < other.it; }

protected:
	IterTy it;
	WrappedIterator(IterTy it)
		: it(it)
	{}
private:
	Derived* DerivedThis() { return static_cast<Derived*>(this); }
	const Derived* DerivedThis() const { return static_cast<const Derived*>(this); }
	Derived* DerivedThisUnconst() const { return const_cast<Derived*>(this); }
};

template<class IterTy, class Func>
class MapIter : public WrappedIterator<
	MapIter<IterTy, Func>, IterTy,
	typename std::result_of<Func(typename IterTy::value_type)>::type>
{
    //work around dependent base lookup rules (C++FAQ 35.19)
    using Base = WrappedIterator<MapIter<IterTy, Func>, IterTy,
	typename std::result_of<Func(typename IterTy::value_type)>::type>;
public:
	MapIter(IterTy it, Func f)
		: Base(it), func(f)
	{}

	typename Base::value_type operator*() { return func(*Base::it); }

    // ???
	//template<class dummy = void>
	//typename Base::value_type* operator->() { static_assert(false, "MapIter::operator-> is undefined"); }

private:
	Func func;
};

template<class IterTy, class Func>
MapIter<IterTy, Func> MakeMapIter(IterTy it, Func f)
{
	return{ it, f };
}

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
	ret size()
	{
		return end_ - begin_;
	}

private:
	iterator begin_, end_;
};

template<class Iter>
range<Iter> make_range(Iter b, Iter e) { return{ b, e }; }


template<class RangeTy, class Func>
auto MapRange(RangeTy& r, Func f) -> range<MapIter<decltype(std::begin(r)), Func>>
{
	return{ { std::begin(r), f }, { std::end(r), f } };
}

#endif
