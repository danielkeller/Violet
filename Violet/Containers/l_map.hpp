#ifndef L_MAP_HPP
#define L_MAP_HPP

#include <functional>
#include <memory>
#include "WrappedIterator.hpp"

template<class Key, class T,
    class Compare = std::less<Key>,
    class Alloc = std::allocator<std::pair<const Key, T>>>
class l_map
{
public:
    using key_type = Key;
    using mapped_type = T;
	//const causes iterator issues with MSVC
	using value_type = std::pair</*const*/ Key, T>;
	using size_type = typename std::vector<value_type, Alloc>::size_type;
	using difference_type = typename std::vector<value_type, Alloc>::difference_type;
private:
	using storety = std::vector<value_type, Alloc>;
	using indty = const std::pair<Key, difference_type>;
    using indsty = std::vector<indty>;
public:

	l_map() = default;
	l_map(std::initializer_list<value_type> init)
	{
		for (auto& v : init)
			emplace_back(v);
	}
	l_map(l_map&& other)
		: store(std::move(other.store))
        , inds(std::move(other.inds))
	{}
	l_map(const l_map& other)
		: store(other.store)
        , inds(other.inds)
	{}

    l_map& operator=(l_map other)
    {
        swap(store, other.store);
        swap(inds, other.inds);
        return *this;
    }

private:
    struct Comparer
    {
		bool operator()(const indty& l, const key_type& r)
            { return Compare()(l.first, r); };
    };
public:

	using iterator = typename storety::iterator;
	using const_iterator = typename storety::const_iterator;

	iterator begin() {return store.begin();}
	iterator end() {return store.end();}
	const_iterator begin() const {return store.cbegin();}
	const_iterator end() const {return store.cend();}
	const_iterator cbegin() const {return store.cbegin();}
	const_iterator cend() const {return store.cend();}

	size_type size() const { return store.size(); }
	value_type* data() { return store.data(); }
	const value_type* data() const { return store.data(); }

	//be orthogonal to l_bag
	using perma_ref = key_type;

	iterator find(const key_type& k)
    {
        auto pos = find_ind(k);
        if(pos == inds.end() || pos->first != k)
            return store.end();
        else
            return store.begin() + pos->second;
    }

	const_iterator find(const key_type& r) const
    {
        return const_cast<l_map*>(this)->at(r);
    }

	template<class... Args>
	bool try_emplace_back(const key_type& key, Args&&... args)
	{
        return try_emplace(store.end(), key, std::forward<Args>(args)...);
	}

    //this should be const_iterator but libstdc++ has a bug
	template<class... Args>
	std::pair<iterator, bool> try_emplace(iterator pos, const key_type& key, Args&&... args)
	{
        auto indit = find_ind(key);
        if (indit != inds.end() && indit->first == key)
            return std::make_pair(pos, false);
        for (auto& ind : inds)
            if (ind.second >= pos - store.begin()) ++ind.second;
        pos = store.emplace(pos, std::piecewise_construct,
			std::forward_as_tuple(key),
            std::forward_as_tuple(std::forward<Args>(args)...));
        inds.emplace(indit, key, pos - store.begin());
		return std::make_pair(pos, true);
	}

    iterator erase(const_iterator pos)
    {
        auto indit = find_ind(pos->first);
        auto my_ind = pos - store.begin();
        for(auto& ind : inds)
            if (ind.second > my_ind) --ind.second; //shift over ones to the right
        inds.erase(indit);
        return store.erase(pos);
	}

	size_type erase(const key_type& key)
	{
		auto it = find(key);
		if (it == end())
			return 0;
		erase(it);
		return 1;
	}

	mapped_type& operator[](const key_type& key)
	{
		auto it = find(key);
		if (it == end())
		{
			try_emplace_back(key);
			return (end() - 1)->second;
		}
		return it->second;
	}

private:
    typename indsty::iterator find_ind(const key_type& k)
    {
        return std::lower_bound(inds.begin(), inds.end(), k, Comparer());
    }

	storety store;
    indsty inds;
};

#endif
