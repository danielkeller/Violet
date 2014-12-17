#ifndef BINTREE_HPP
#define BINTREE_HPP
#include <vector>
#include <array>
#include "WrappedIterator.hpp"

//default traversal order is breadth-first
template<class NodeTy, class LeafTy, class ContTy>
class BinTreeIterBase :
	public WrappedIterator < BinTreeIterBase<NodeTy, LeafTy, ContTy>, size_t, NodeTy >
{
    using Base = WrappedIterator<BinTreeIterBase<NodeTy, LeafTy, ContTy>, size_t, NodeTy>;
    using Base::it;
    friend Base;
public:
	NodeTy& operator*()
	{
		tree.ReserveNodes(it);
		return tree.inner[it];
	}
	using Base::operator*;

	void ToLeft() { it = 2 * it + 1; }
	void ToRight() { it = 2 * it + 2; }
	void ToParent() { it = (it-1)/2; }
	BinTreeIterBase Left() const { auto ret = *this; ret.ToLeft(); return ret; }
	BinTreeIterBase Right() const { auto ret = *this; ret.ToRight(); return ret; }
	BinTreeIterBase Parent() const { auto ret = *this; ret.ToParent(); return ret; }
	bool Bottom() const { return Left() > tree.end(); }
	bool Top() const { return it == 0; }
	size_t Depth() const
	{
		auto iter = *this;
		size_t lvl = 0;
		while (!iter.Top()) { iter.ToParent(); ++lvl; }
		return lvl;
	}

private:
	friend ContTy;
	ContTy& tree;

	BinTreeIterBase(size_t pos, ContTy& t)
		: Base(pos), tree(t)
	{}

	size_t Leaf() const { return it - tree.inner.size() / 2; }
};

template<class NodeTy, class LeafTy,
class NodeAlloc = std::allocator<NodeTy>, class LeafAlloc = std::allocator<LeafTy>>
class BinTree
{
public:
	BinTree(NodeTy&& n)
	{
		inner.push_back(std::move(n));
	}

	using iterator = BinTreeIterBase < NodeTy, LeafTy, BinTree >;
	using const_iterator = BinTreeIterBase <const NodeTy, const LeafTy, BinTree>;

	iterator begin() { return{ 0, *this }; }
	const_iterator begin() const { return{ 0, *const_cast<BinTree*>(this) }; }
	const_iterator cbegin() { return begin(); }

	iterator end() { return{ inner.size(), *this }; }
	const_iterator end() const { return{ inner.size(), *const_cast<BinTree*>(this) }; }
	const_iterator cend() { return end(); }

	//Only guaranteed to work if the tree is being built preorder
	void PreorderLeafPush(LeafTy leaf)
	{
		leaves.push_back(leaf);
	}

	//Each bottom node has one leaf
	LeafTy& Leaf(const iterator& parent)
	{
		//Leaf numbering starts at the first node with no children
		ReserveLeaves(parent.Leaf());
		return leaves[parent.Leaf()];
	}

	const LeafTy& Leaf(const const_iterator& parent) const
	{
		return leaves[parent.Leaf()];
	}

private:
	friend iterator;
	friend const_iterator;

	std::vector<NodeTy, NodeAlloc> inner;
	std::vector<LeafTy, LeafAlloc> leaves;

	void ReserveLeaves(size_t idx)
	{
		if (leaves.size() <= idx)
			leaves.resize(idx + 1);
	}

	void ReserveNodes(size_t idx)
	{
		if (inner.size() <= idx)
		{
			//TODO this is inefficient
			if (leaves.size() >= idx - inner.size())
				leaves.erase(leaves.begin(), leaves.begin() + idx - inner.size());
			inner.resize(idx + 1);
		}
	}
};

#endif
