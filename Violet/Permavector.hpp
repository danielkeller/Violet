#ifndef PERMAVECTOR_HPP
#define PERMAVECTOR_HPP
#include <vector>
#include <WrappedIterator.hpp>

//Vector with iterator-like objects that can't be invalidated
template<class T, class Alloc = std::allocator<std::pair<int, T>>>
class Permavector
{
public:
	using value_type = std::pair<T, int>;
private:
	using storety = std::vector<value_type, Alloc>;
public:

	Permavector() = default;
	Permavector(std::initializer_list<T> init)
	{
		for (auto& v : init)
			emplace_back(v);
	}
	Permavector(Permavector&& other)
		: store(std::move(other.store))
	{}
	Permavector(const Permavector& other)
		: store(other.store)
	{}

	template<class Val, class It>
	class iterator_base : public WrappedIterator<iterator_base<Val, It>, It, Val>
	{
        using Base = WrappedIterator<iterator_base<Val, It>, It, Val>;
public:
		Val& operator*() { return Base::it->first; }
private:
		iterator_base(It it) : Base(it) {}
		friend class Permavector;
	};

	using iterator = iterator_base < T, typename storety::iterator >;
	using const_iterator = iterator_base < const T, typename storety::const_iterator >;

	iterator begin()
	{
		return iterator{ store.begin() };
	}

	iterator end()
	{
		return iterator{ store.end() };
	}

	const_iterator begin() const
	{
		return const_iterator{ store.cbegin() };
	}

	const_iterator end() const
	{
		return const_iterator{ store.cend() };
	}

	using size_type = typename storety::size_type;
	size_type size() const { return store.size(); }
	value_type* data() { return store.data(); }
	const value_type* data() const { return store.data(); }

	class perma_ref
	{
	public:
		iterator get(Permavector& vec)
		{
			auto iter = std::lower_bound(vec.store.begin(), vec.store.end(), it,
				[](const typename storety::value_type& elem, int val){
				return elem.second < val;
			});
			return iterator{ iter };
		}

		const_iterator get(Permavector& vec) const
		{
			return const_cast<perma_ref*>(this)->get(vec);
		}

		perma_ref(const perma_ref& other) : it(other.it) {}
		perma_ref& operator=(perma_ref other)
		{
			it = other.it;
			return *this;
		}

	private:
		perma_ref(int it) : it(it) {}
		int it;
		friend class Permavector;
	};

	template<class... Args>
	perma_ref emplace_back(Args&&... args)
	{
		int next = store.empty() ? 0 : store.back().second + 1;
		store.emplace_back(std::piecewise_construct,
			std::forward_as_tuple(args...), std::make_tuple(next));
		return perma_ref{ next };
	}

private:
	storety store;
	friend class perma_iterator;
};

#endif
