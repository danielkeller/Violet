#ifndef BINTREE_HPP
#define BINTREE_HPP
#include <array>
#include <deque>
#include "variant.hpp"
#include "Utils/Profiling.hpp"
#include "Utils/Template.hpp"

template<class N> class BinTree;

template<class N>
class BinTreeIterBase
{
protected:
    size_t pos;
    using NodeT = std::remove_const_t<N>;
    using ContTy = std::vector<NodeT>;
    ContTy* nodes;
    
    BinTreeIterBase(size_t pos, ContTy* t) : pos(pos), nodes(t) {}
    //the interface enforces constness
    BinTreeIterBase(size_t pos, const ContTy* t) : BinTreeIterBase(pos, const_cast<ContTy*>(t)) {}
    
    template<class N1> friend class BinTree;
    
public:
    BinTreeIterBase() = default;
    
	N& operator*()
    {
        //support inserting this way
        if (nodes->size() <= pos)
            nodes->resize(pos + 1);
        return (*nodes)[pos];
    }
    
    N* operator->() { return &**this; }

	void ToLeft() { pos = 2 * pos + 1; }
	void ToRight() { pos = 2 * pos + 2; }
	void ToParent() { pos = (pos-1)/2; }
	BinTreeIterBase Left() const { auto ret = *this; ret.ToLeft(); return ret; }
	BinTreeIterBase Right() const { auto ret = *this; ret.ToRight(); return ret; }
    BinTreeIterBase Parent() const { auto ret = *this; ret.ToParent(); return ret; }
    explicit operator bool() const { return pos < nodes->size() && (*nodes)[pos]; }
    bool Bottom() const { return !Left() && !Right(); }
	bool Top() const { return pos == 0; }
	size_t Depth() const
	{
		auto iter = *this;
		size_t lvl = 0;
		while (!iter.Top()) { iter.ToParent(); ++lvl; }
		return lvl;
	}
    
    bool operator==(BinTreeIterBase other) { return pos == other.pos; }
    bool operator!=(BinTreeIterBase other) { return pos != other.pos; }
        
    BinTreeIterBase& operator++()
    {
        //skip empty nodes
        while (pos < nodes->size() && !*this) ++pos;
        return *this;
    }
    BinTreeIterBase& operator--()
    {
        while (pos < 0 && !*this) --pos;
        return *this;
    }
    
    BinTreeIterBase operator++(int)
    {
        auto ret = *this;
        ++*this;
        return ret;
    }
    BinTreeIterBase operator--(int)
    {
        auto ret = *this;
        --*this;
        return ret;
    }
};

template<class N>
class BinTree
{
public:
    using value_type = nullable_t<N>;
    
	BinTree() = default;
	BinTree(const BinTree&) = default;
	BinTree(BinTree&&) = default;
	BinTree& operator=(BinTree other)
	{
		std::swap(nodes, other.nodes);
		return *this;
	}

	using iterator = BinTreeIterBase<value_type>;
	using const_iterator = BinTreeIterBase<const value_type>;

	iterator begin() { return{ 0, &nodes }; }
	const_iterator begin() const { return{ 0, &nodes }; }
	const_iterator cbegin() const { return{ 0, &nodes }; }

	iterator end() { return{ nodes.size(), &nodes }; }
	const_iterator end() const { return{ nodes.size(), &nodes }; }
	const_iterator cend() const { return{ nodes.size(), &nodes }; }
    
    void insert(iterator pos, value_type node)
    {
        if (nodes.size() <= pos.pos)
            nodes.resize(pos.pos + 1);
        nodes[pos.pos] = node;
    }
    
    void reserve(size_t height)
    {
        nodes.reserve((1 << height) - 1);
    }
    
    size_t size() const
    {
        size_t s = 0;
        for (auto& n : nodes) if (n) ++s;
        return s;
    }
    
    size_t capacity() const
    {
        return nodes.capacity();
    }

private:
    template<class N1> friend class BinTreeIterBase;

	std::vector<value_type> nodes;
};

#endif
