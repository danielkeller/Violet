#ifndef L_MAP_HPP
#define L_MAP_HPP

#include <functional>
#include <memory>
#include "WrappedIterator.hpp"

//alternative: implement this with unordered_map and l_bag
//pros: O(1) lookup
//cons: more memory

template<class Key, class T,
    class Compare = std::less<Key>,
    class Alloc = std::allocator<std::pair<const Key, T>>>
class l_map
{
public:
    using key_type = Key;
    using mapped_type = T;
	using value_type = std::pair<const Key, T>;
	using size_type = typename std::vector<value_type, Alloc>::size_type;
private:
	using storety = std::vector<value_type, Alloc>;
    using indty = const std::pair<Key, size_type>;
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
    template<class Iter>
    class IterBase : public WrappedIterator<IterBase<Iter>, Iter, typename Iter::value_type::second_type>
    {
        using Base = WrappedIterator<IterBase<Iter>, Iter, typename Iter::value_type::second_type>;
    public:
        using Base::operator*;
        typename Base::reference operator*() {return Base::it->second;}
    };

    class Comparer
    {
        bool operator()(const key_type& l, const indty& r)
            { return Compare()(l, r.first); };
    };
public:

	using iterator = IterBase<typename storety::iterator>;
	using const_iterator = IterBase<typename storety::const_iterator>;

	iterator begin() {return store.begin();}
	iterator end() {return store.end();}
	const_iterator begin() const {return store.cbegin();}
	const_iterator end() const {return store.cend();}
	const_iterator cbegin() const {return store.cbegin();}
	const_iterator cend() const {return store.cend();}

	size_type size() const { return store.size(); }
	value_type* data() { return store.data(); }
	const value_type* data() const { return store.data(); }

    iterator at(const key_type& k)
    {
        auto pos = find_ind(k);
        if(pos == inds.end() || pos->first != k)
            return store.end();
        else
            return store.begin() + pos->second;
    }

    const_iterator at(const key_type& r) const
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
	bool try_emplace(iterator pos, const key_type& key, Args&&... args)
	{
        auto indit = find_ind(key);
        if (indit != inds.end() && indit->first == key)
            return false;
        for (auto& ind : inds)
            if (ind.second > pos - store.begin()) ++ind.second;
        pos = store.emplace(pos, std::piecewise_construct, key, 
            std::forward_as_tuple(std::forward<Args>(args)...));
        inds.emplace(indit, key, pos - store.begin());
        return true;
	}

    iterator erase(const_iterator pos)
    {
        auto indit = find_ind(pos->first);
        auto my_ind = pos - store.begin();
        for(auto& ind : inds)
            if (ind > my_ind) --ind->second; //shift over ones to the right
        inds.erase(indit);
        return store.erase(pos);
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