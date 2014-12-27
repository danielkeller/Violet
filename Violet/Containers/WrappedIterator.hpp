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
    using reference = value_type&;

	template<class dummy = void> //Don't compile this if it doesn't make sense
	reference operator*() { return *it; }
	const reference operator*() const { return *DerivedThisUnconst(); }
	pointer operator->() { return &**DerivedThis(); }
	const pointer operator->() const { return &**DerivedThisUnconst(); }
	
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

#endif
