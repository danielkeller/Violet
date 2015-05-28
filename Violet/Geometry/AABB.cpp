#include "stdafx.h"
#include "AABB.hpp"
//#include <boost/range/algorithm.hpp>
#include <numeric>

AABB::AABB(Mesh m)
	: tree(7, Bound(m), m,
		[](Mesh parent, const AlignedBox3f& cur)
		{
			//Get longest axis
			Vector3f::Index longestAxis;
			cur.sizes().maxCoeff(&longestAxis);

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
		},
		[](Mesh mLeft, const AlignedBox3f&)
		{
			return mLeft;
		}
		)
{}

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

	for (auto it = aabb.tree.begin(); it != aabb.tree.end(); ++it)
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
