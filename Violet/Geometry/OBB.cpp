#include "stdafx.h"
#include "OBB.hpp"
#include "Core/Resource.hpp"
#include "AABB.hpp"

#include <iostream>

struct OBBTree::Resource : public ::Resource<OBBTree::Resource>
{
	TreeTy tree;
	Resource(std::string file);
	void LoadCache(std::string cacheFile);
	void SaveCache(std::string cacheFile);
};

OBBTree::Resource::Resource(std::string file)
	: ResourceTy(file)
{
	auto p = Profile("build OBB");

	Mesh m = LoadMesh(file);

	float volTotal = 0.f, imbTotal = 0.f;

	static const int MAX_TRIS_PER_LEAF = 3;
	size_t height = static_cast<size_t>(
		std::max(1.f, std::ceilf(std::log2f(float(m.size() / MAX_TRIS_PER_LEAF)))));

	//constrct the OBB tree
	tree = TreeTy::TopDown(height, m, m,
		[&](Mesh parent, const OBB& cur)
	{
		if (cur.volume() > 0)
			volTotal += cur.volume();

		//Get longest axis
		Vector3f::Index longestAxisIndex;
		cur.axes.colwise().norm().maxCoeff(&longestAxisIndex);

		Vector3f meshCentroid = centroid(parent);

		Mesh left, right;

		auto axInd = longestAxisIndex;

		while (true)
		{
			Vector3f ax = cur.axes.col(axInd).normalized();

			std::partition_copy(parent.begin(), parent.end(),
				std::back_inserter(left), std::back_inserter(right),
				[=](const Triangle& t)
			{
				return ::centroid(t).dot(ax) < meshCentroid.dot(ax);
			});

			axInd = (axInd + 1) % 3;

			//If one side has the whole mesh, split it differently
			if (left.size() != 0 && right.size() != 0 //nondegenerate split
				|| axInd == longestAxisIndex) //no more axes to try
				break;

			//still axes to try
			left.clear(), right.clear();
		}

		if (left.size() + right.size() > 0)
			imbTotal += std::abs(float(left.size()) - float(right.size()))
				/ float(left.size() + right.size());

		//be a little less efficient to make things easier
		if (left.size() == 0)
			left = right;
		if (right.size() == 0)
			right = left;

		OBB lBound{ left };
		OBB rBound{ right };

		return std::make_tuple(
			lBound, std::move(left),
			rBound, std::move(right));

	},
		[](Mesh mLeft, const OBB&)
	{
		return mLeft;
	}
	);
	
	std::cerr << "Total volume for " << file << ' ' << volTotal <<
		" imbalace " << imbTotal / float(tree.end() - tree.begin()) << '\n';
}

OBBTree::OBBTree(std::string file)
	: resource(Resource::FindOrMake(file))
{}

std::string OBBTree::Name() const
{
	return resource->Key();
}

const OBBTree::TreeTy& OBBTree::Tree() const
{
	return resource->tree;
}
