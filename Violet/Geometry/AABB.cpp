#include "stdafx.h"
#include "AABB.hpp"
#include <numeric>

//These methods take comparable time to produce a tree of comparable size
//#define OVERLAP_OK

//#include <iostream>
//static float volTotal = 0.f;

AABB::AABB(Mesh m)
	: tree(8, Bound(m), m,
		[](Mesh parent, const AlignedBox3f& cur)
		{
			//if (std::isfinite(cur.volume()))
			//	volTotal += cur.volume();

			//Get longest axis
			Vector3f::Index longestAxis;
			cur.sizes().maxCoeff(&longestAxis);

#ifdef OVERLAP_OK
			auto rCenter = MapRange(parent, centroid);
			Vector3f meanCentroid =
				std::accumulate(rCenter.begin(), rCenter.end(), Vector3f(0, 0, 0))
				/ float(parent.size());

			Mesh left, right;

			std::partition_copy(parent.begin(), parent.end(),
				std::back_inserter(left), std::back_inserter(right),
				[=](const Triangle& t)
			{
				return t.row(longestAxis).maxCoeff() < meanCentroid[longestAxis];
				//produces much higher total volume
				//return centroid(t)[longestAxis] < meanCentroid[longestAxis];
			});

			auto lBound = Bound(left);
			auto rBound = Bound(right);

			return std::make_tuple(
				lBound, std::move(left),
				rBound, std::move(right));

#else
			auto rCenter = MapRange(parent, centroid);
			Vector3f meanCentroid =
				std::accumulate(rCenter.begin(), rCenter.end(), Vector3f(0, 0, 0))
				/ float(parent.size());

			AlignedBox3f left = cur, right = cur;
			left.max()[longestAxis] = meanCentroid[longestAxis];
			right.min()[longestAxis] = meanCentroid[longestAxis];

			//remove all non-intersecting elements
			Mesh mLeft = ApproxChop(parent, left);
			Mesh mRight = ApproxChop(std::move(parent), right);
			return std::make_tuple(left.intersection(Bound(mLeft)), std::move(mLeft),
				right.intersection(Bound(mRight)), std::move(mRight));
#endif
		},
		[](Mesh mLeft, const AlignedBox3f&)
		{
			return mLeft;
		}
		)
{
	//std::cerr << volTotal << '\n';
}

struct AABBVert
{
	//Prevent alignment issues from causing asserts
	using Allocator = Eigen::aligned_allocator<AABBVert>;

	Vector3f pos;
	Vector3f color;
};

template<>
const Schema AttribTraits<AABBVert>::schema = {
	AttribProperties{ "position", GL_FLOAT, false, 0,                 {3, 1}},
	AttribProperties{ "color",    GL_FLOAT, false, 3 * sizeof(float), {3, 1}},
};

ShowAABB::ShowAABB(const AABB& aabb)
	: shaderProgram ("assets/color")
{
	std::vector<AABBVert, AABBVert::Allocator> attribs;
	std::vector<LineInd> indices;

	auto it = aabb.tree.begin();
	//while (!it.Bottom()) ++it;

	for (; it != aabb.tree.end(); ++it)
	{
		auto depth = static_cast<float>(it.Depth());
		Vector3f color = {
			std::sin(2.f*PI_F*depth / 6.f) / 2.f + .5f,
			std::sin(2.f*PI_F*depth / 6.f + 2) / 2.f + .5f,
			std::sin(2.f*PI_F*depth / 6.f + 4) / 2.f + .5f
		};

		auto ind = static_cast<GLint>(attribs.size());

		attribs.emplace_back(AABBVert{ it->corner(AlignedBox3f::BottomLeft),  color });
		attribs.emplace_back(AABBVert{ it->corner(AlignedBox3f::BottomRight), color });
		attribs.emplace_back(AABBVert{ it->corner(AlignedBox3f::TopRight),     color });
		attribs.emplace_back(AABBVert{ it->corner(AlignedBox3f::TopLeft),    color });

		attribs.emplace_back(AABBVert{ it->corner(AlignedBox3f::BottomLeftCeil),  color });
		attribs.emplace_back(AABBVert{ it->corner(AlignedBox3f::BottomRightCeil), color });
		attribs.emplace_back(AABBVert{ it->corner(AlignedBox3f::TopRightCeil),     color });
		attribs.emplace_back(AABBVert{ it->corner(AlignedBox3f::TopLeftCeil),    color });

		indices.insert(indices.end(), {
			{ ind, ind + 1 }, { ind + 1, ind + 2 }, { ind + 2, ind + 3 }, { ind + 3, ind },
			{ ind, ind + 4 }, { ind + 1, ind + 5 }, { ind + 2, ind + 6 }, { ind + 3, ind + 7 },
			{ ind + 4, ind + 5 }, { ind + 5, ind + 6 }, { ind + 6, ind + 7 }, { ind + 7, ind + 4 }
		});
	}

	vertData = VertexData::VertexDataResource::MakeShared(
		"#debug#", attribs, indices);
}
