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

	template<class dummy = void> //Don't compile this if it doesn't make sense
	value_type& operator*() { return *it; }
	value_type* operator->() { return &**DerivedThis(); }
	
	Derived& operator++() { ++it; return *DerivedThis(); }
	Derived operator++(int) const { auto ret = *DerivedThis(); ++ret; return ret; }
	Derived& operator--() { --it; return *DerivedThis(); }
	Derived operator--(int) const { auto ret = *DerivedThis(); --ret; return ret; }
	Derived& operator+=(ptrdiff_t o) { it += o; return *DerivedThis(); }
	Derived operator+(ptrdiff_t o) const { auto ret(*DerivedThis()); ret.it = it + o; return ret; }
	Derived operator-(ptrdiff_t o) const { auto ret(*DerivedThis()); ret.it = it - o; return ret; }
	int operator-(Derived& other) const { return it - other.it; }
	
	bool operator==(Derived& other) const { return it == other.it; }
	bool operator!=(Derived& other) const { return it != other.it; }
	bool operator<(Derived& other) const { return it < other.it; }

protected:
	IterTy it;
	WrappedIterator(IterTy it)
		: it(it)
	{}
private:
	Derived* DerivedThis() { return static_cast<Derived*>(this); }
	const Derived* DerivedThis() const { return static_cast<const Derived*>(this); }
};

template<class IterTy, class Func>
class MapIter : public WrappedIterator <
	MapIter<IterTy, Func>, IterTy,
	typename std::result_of<Func(typename IterTy::value_type)>::type>
{
public:
	MapIter(IterTy it, Func f)
		: WrappedIterator(it), func(f)
	{}

	value_type operator*() { return func(*it); }

	template<class dummy = void>
	value_type* operator->() { static_assert(false, "MapIter::operator-> is undefined"); }

private:
	Func func;
};


template<class IterTy, class Func>
MapIter<IterTy, Func> MakeMapIter(IterTy it, Func f)
{
	return{ it, f };
}

#endif