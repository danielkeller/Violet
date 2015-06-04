#ifndef L_UNORDMAP_HPP
#define L_UNORDMAP_HPP

#include <unordered_map>
#include "WrappedIterator.hpp"

template<class Key, class T,
class Hash = std::hash<Key>,
class KeyEqal = std::equal_to<Key>,
class Alloc = std::allocator<std::pair<Key, T>>>
class l_unordered_map
{
public:
	using key_type = Key;
	using mapped_type = T;
	using value_type = std::pair<Key, T>;
	using size_type = typename std::vector<value_type, Alloc>::size_type;
private:
	using storety = std::vector<value_type, Alloc>;
	using indsty = std::unordered_map<Key, size_type, Hash, KeyEqal
		/*use default alloctor*/>;
public:

	l_unordered_map() = default;
	l_unordered_map(std::initializer_list<value_type> init)
	{
		for (auto& v : init)
			emplace_back(v);
	}
	l_unordered_map(l_unordered_map&& other)
		: store(std::move(other.store))
		, inds(std::move(other.inds))
	{}
	l_unordered_map(const l_unordered_map& other)
		: store(other.store)
		, inds(other.inds)
	{}

	l_unordered_map& operator=(l_unordered_map other)
	{
		swap(store, other.store);
		swap(inds, other.inds);
		return *this;
	}

public:
	using iterator = typename storety::iterator;
	using const_iterator = typename storety::const_iterator;

	iterator begin() { return store.begin(); }
	iterator end() { return store.end(); }
	const_iterator begin() const { return store.cbegin(); }
	const_iterator end() const { return store.cend(); }
	const_iterator cbegin() const { return store.cbegin(); }
	const_iterator cend() const { return store.cend(); }

	size_type size() const { return store.size(); }
	value_type* data() { return store.data(); }
	const value_type* data() const { return store.data(); }

	size_type count(const key_type& k) const { return inds.count(k); }

	iterator find(const key_type& k)
	{
		auto pos = inds.find(k);
		if (pos == inds.end())
			return store.end();
		else
			return store.begin() + pos->second;
	}

	const_iterator find(const key_type& r) const
	{
		return const_cast<l_unordered_map*>(this)->find(r);
	}

	const T& at(const key_type& r) const
	{
		auto it = find(r);
		if (it == store.end())
			throw std::domain_error("l_unordered_map does not contain key");
		return it->second;
	}

	T& operator[](const key_type& key)
	{
		return try_emplace(key).first->second;
	}

	template<class... Args>
	std::pair<iterator, bool>
		try_emplace(const key_type& key, Args&&... args)
	{
		auto indit = inds.find(key);
		if (indit != inds.end())
			return std::make_pair(store.begin() + indit->second, false);
		store.emplace_back(std::piecewise_construct, std::tie(key),
			std::forward_as_tuple(std::forward<Args>(args)...));
		inds.emplace(key, store.size() - 1);
		return std::make_pair(store.end() - 1, true);
	}

	iterator erase(iterator pos)
	{
		//swap with last element
		swap(*pos, store.back());
		inds[pos->first] = pos - store.begin();
		//pop off the last element
		inds.erase(store.back().first);
		store.resize(store.size() - 1);
		return pos;
	}

	size_type erase(const key_type& key)
	{
		auto pos = find(key);
		if (pos == end())
			return 0;
		erase(pos);
		return 1;
	}

private:
	storety store;
	indsty inds;
};

#endif
