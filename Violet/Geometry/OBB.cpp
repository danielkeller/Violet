#include "stdafx.h"
#include "OBB.hpp"
#include "Core/Resource.hpp"
#include "AABB.hpp"

#include <numeric>
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

	size_t height = static_cast<size_t>(std::ceilf(std::log2f(float(m.size()))));

	//constrct the AABB tree
	tree = TreeTy::TopDown(height, AABBToObb(Bound(m)), m,
		[&](Mesh parent, const OBB& cur)
	{
		//Get longest axis
		Vector3f::Index longestAxis;
		cur.axes.colwise().norm().maxCoeff(&longestAxis);

		auto rCenter = MapRange(parent, centroid);
		Vector3f meanCentroid =
			std::accumulate(rCenter.begin(), rCenter.end(), Vector3f(0, 0, 0))
			/ float(parent.size());

		Mesh left, right;

		auto ax = longestAxis;

		while (true)
		{
			std::partition_copy(parent.begin(), parent.end(),
				std::back_inserter(left), std::back_inserter(right),
				[=](const Triangle& t)
			{
				return t.row(ax).maxCoeff() < meanCentroid[ax];
				//produces much higher total volume
				//return centroid(t)[ax] < meanCentroid[ax];
			});

			ax = (ax + 1) % 3;

			//If one side has the whole mesh, split it differently
			if (left.size() != 0 && right.size() != 0 //nondegenerate split
				|| ax == longestAxis) //no more axes to try
				break;

			//still axes to try
			left.clear(), right.clear();
		}

		//be a little less efficient to make things easier
		if (left.size() == 0)
			left = right;
		if (right.size() == 0)
			right = left;

		auto lBound = Bound(left);
		auto rBound = Bound(right);

		return std::make_tuple(
			AABBToObb(lBound), std::move(left),
			AABBToObb(rBound), std::move(right));

	},
		[](Mesh mLeft, const OBB&)
	{
		return mLeft[0]; //fixme
	}
	);

	//now fit the nodes better
	auto it = tree.end() - 1;

	//Do the last row
	for (; it.Bottom(); --it)
	{
		Matrix3f tri = tree.Leaf(it);

		Vector3f areas;

		//http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
		for (int pt = 0; pt < 3; ++pt)
			areas[pt] = (tri.col(pt) - tri.col((pt+1) % 3))
				.cross(tri.col(pt) - tri.col((pt+2) % 3)).norm();

		//how thick flat things are
		static const float ZERO_SIZE = 0.001f;

		if (!areas.isZero())
		{
			Vector3f::Index bestPt;
			areas.minCoeff(&bestPt);

			it->origin = tri.col((bestPt + 1) % 3);
			//use the opposite side
			it->axes.col(0) = tri.col((bestPt + 2) % 3) - it->origin;
			//now in the direction of the point
			Vector3f bestDir = tri.col(bestPt) - it->origin;
			Vector3f firstDir = it->axes.col(0).normalized();
			//project out the common component
			it->axes.col(1) = bestDir - firstDir * bestDir.dot(firstDir);
			it->axes.col(2) = bestDir.cross(firstDir).normalized() * ZERO_SIZE;
		}
		else //degenerate triangle
		{
			Matrix3f sides;
			for (int pt = 0; pt < 3; ++pt)
				sides.col(pt) = tri.col((pt+1) % 3) - tri.col(pt);

			Vector3f::Index bestPt; //longest side
			sides.colwise().norm().maxCoeff(&bestPt);

			it->origin = tri.col(bestPt);
			it->axes = Matrix3f::Identity() * ZERO_SIZE;

			if (!sides.isZero()) //line segment
			{
				//set the X axis to the correct length
				it->axes.col(0) = Vector3f{ sides.col(bestPt).norm(), 0, 0 };
				//rotate X into the line segment
				it->axes *= Quaternionf::FromTwoVectors(Vector3f::UnitX(),
					sides.col(bestPt)).matrix();
			}
		}
	}

	do {
		*it = MergeFace(*it.Left(), *it.Right());
		//real reverse iterators would be nice
	} while (it-- != tree.begin());
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
