#include "stdafx.h"
#include "AABB.hpp"
//#include <boost/range/algorithm.hpp>
#include <numeric>

AABB::AABB(Mesh m)
	: tree(m.bound())
{
	build(m, m.bound(), 0, tree.begin());
}

Vector3f centroid(const Triangle& t)
{
	return (t.q + t.r + t.s) / 3.f;
}

void AABB::build(Mesh mLeft, Box cur, size_t depth, TreeTy::iterator it)
{
	//auto origSize = mLeft.size()
	*it = cur;

	if (mLeft.size() <= 6 || depth > 6)
	{
		tree.PreorderLeafPush(mLeft);
		return;
	}

	//Get longest axis
	auto lengths = (cur.b - cur.a).array();
	auto longestAx = Eigen::Array3f::Constant(lengths.abs().maxCoeff()) == lengths;

	auto centrBegin = MakeMapIter(mLeft.cbegin(), centroid);
	auto centrEnd = MakeMapIter(mLeft.cend(), centroid);
	auto meanCentroid = (std::accumulate(centrBegin, centrEnd, Vector3f(0, 0, 0))
		/ float(mLeft.size())).eval();

	Box left = cur, right = cur;
	for (int ind = 0; ind < 3; ++ind)
	{
		if (longestAx[ind])
		{
			left.b[ind] = meanCentroid[ind];
			right.a[ind] = meanCentroid[ind];
			break; //in case some axes are identical
		}
	}

	Mesh mRight = mLeft;
	//remove all non-intersecting elements
	mLeft.Chop(left);
	mRight.Chop(right);
	build(mLeft, left & mLeft.bound(), depth + 1, it.Left());
	build(mRight, right & mRight.bound(), depth + 1, it.Right());

	//dq += (mLeft.size() + mRight.size() - origSize)/origSize
}

struct AABBVert
{
	//Prevent alignment issues from causing asserts
	using Allocator = Eigen::aligned_allocator<AABBVert>;

	Vector3f pos;
	Vector3f color;
	static const Schema schema;
};

const Schema AABBVert::schema = {
		{ "position", 3, GL_FLOAT, 0, 1 },
		{ "color", 3, GL_FLOAT, 3 * sizeof(float), 1 },
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

		attribs.emplace_back(AABBVert{ it->a, color });
		attribs.emplace_back(AABBVert{ { it->a[0], it->a[1], it->b[2] }, color });
		attribs.emplace_back(AABBVert{ { it->a[0], it->b[1], it->b[2] }, color });
		attribs.emplace_back(AABBVert{ { it->a[0], it->b[1], it->a[2] }, color });

		attribs.emplace_back(AABBVert{ { it->b[0], it->a[1], it->a[2] }, color });
		attribs.emplace_back(AABBVert{ { it->b[0], it->a[1], it->b[2] }, color });
		attribs.emplace_back(AABBVert{ it->b, color });
		attribs.emplace_back(AABBVert{ { it->b[0], it->b[1], it->a[2] }, color });

		indices.insert(indices.end(), {
			{ ind, ind + 1 }, { ind + 1, ind + 2 }, { ind + 2, ind + 3 }, { ind + 3, ind },
			{ ind, ind + 4 }, { ind + 1, ind + 5 }, { ind + 2, ind + 6 }, { ind + 3, ind + 7 },
			{ ind + 4, ind + 5 }, { ind + 5, ind + 6 }, { ind + 6, ind + 7 }, { ind + 7, ind + 4 }
		});
	}

	vertData = VertexData_detail::VertexDataResource::MakeShared(
		"#debug#", BufferObject<AABBVert, GL_ARRAY_BUFFER, GL_STATIC_DRAW>(attribs),
		BufferObject<LineInd, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW>(indices));
}
