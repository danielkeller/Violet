#ifndef BINTREE_HPP
#define BINTREE_HPP
#include <array>
#include <deque>
#include "WrappedIterator.hpp"
#include "Utils/Profiling.hpp"

//default traversal order is breadth-first
template<class NodeTy, class LeafTy, class ContTy>
class BinTreeIterBase :
	public WrappedIterator<BinTreeIterBase<NodeTy, LeafTy, ContTy>, size_t, NodeTy>
{
    using Base = WrappedIterator<BinTreeIterBase<NodeTy, LeafTy, ContTy>, size_t, NodeTy>;
    using Base::it;
    friend Base;
public:
	NodeTy& operator*()
	{
		return tree->nodes[it];
	}
	using Base::operator*;

	void ToLeft() { it = 2 * it + 1; }
	void ToRight() { it = 2 * it + 2; }
	void ToParent() { it = (it-1)/2; }
	BinTreeIterBase Left() const { auto ret = *this; ret.ToLeft(); return ret; }
	BinTreeIterBase Right() const { auto ret = *this; ret.ToRight(); return ret; }
	BinTreeIterBase Parent() const { auto ret = *this; ret.ToParent(); return ret; }
	bool Bottom() const { return Left() >= tree->end(); }
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
	ContTy* tree;

	BinTreeIterBase(size_t pos, ContTy* t)
		: Base(pos), tree(t)
	{}

	size_t Leaf() const { return it - tree->nodes.size() / 2; }
};

template<class NodeTy, class LeafTy,
class NodeAlloc = std::allocator<NodeTy>, class LeafAlloc = std::allocator<LeafTy>>
class BinTree
{
public:
	BinTree() = default;
	BinTree(const BinTree&) = default;
	BinTree(BinTree&&) = default;
	BinTree& operator=(BinTree other)
	{
		std::swap(nodes, other.nodes);
		std::swap(leaves, other.leaves);
		return *this;
	}

	using iterator = BinTreeIterBase<NodeTy, LeafTy, BinTree>;
	using const_iterator = BinTreeIterBase<const NodeTy, const LeafTy, const BinTree>;

	iterator begin() { return{ 0, this }; }
	const_iterator begin() const { return{ 0, this }; }
	const_iterator cbegin() { return{ 0, this }; }

	iterator end() { return{ nodes.size(), this }; }
	const_iterator end() const { return{ nodes.size(), this }; }
	const_iterator cend() { return{ nodes.size(), this }; }

	template<typename State, typename F, typename G>
	BinTree(size_t height, NodeTy root, State init, F makeNode, G makeLeaf)
	{
		size_t numNodes = (1 << height) - 1;

		nodes.reserve(numNodes);

		std::deque<State> states;
		states.push_back(std::move(init));
		nodes.push_back(std::move(root));

		size_t numInnerNodes = (1 << (height - 1)) - 1;
		for (size_t i = 0; i < numInnerNodes; ++i)
		{
			std::tuple<NodeTy, State, NodeTy, State> tup
				= makeNode(std::move(states.front()), nodes[i]);
			states.pop_front();

			nodes.emplace_back(std::move(std::get<0>(tup)));
			states.emplace_back(std::move(std::get<1>(tup)));
			nodes.emplace_back(std::move(std::get<2>(tup)));
			states.emplace_back(std::move(std::get<3>(tup)));
		}

		size_t numLeaves = 1 << (height - 1);
		leaves.reserve(numLeaves);

		for (size_t i = numInnerNodes; i < numNodes; ++i)
		{
			leaves.emplace_back(makeLeaf(std::move(states.front()), nodes[i]));
			states.pop_front();
		}
	}

	//Each bottom node has one leaf
	LeafTy& Leaf(const iterator& parent)
	{
		return leaves[parent.Leaf()];
	}

	const LeafTy& Leaf(const const_iterator& parent) const
	{
		return leaves[parent.Leaf()];
	}

	size_t Height() const
	{
		size_t h = 0;
		while ((1_sz << h) < nodes.size()) ++h;
		return h;
	}

private:
	friend iterator;
	friend const_iterator;

	std::vector<NodeTy, NodeAlloc> nodes;
	std::vector<LeafTy, LeafAlloc> leaves;
};

#endif
