#include "stdafx.h"
#include "AABB.hpp"
#include "File/Filesystem.hpp"
#include "File/BlobFile.hpp"

#include <numeric>
#include <iostream>

struct AABBTree::Resource : public ::Resource<AABBTree::Resource>
{
	TreeTy tree;
	Resource(std::string file);
	void LoadCache(std::string cacheFile);
	void SaveCache(std::string cacheFile);
};

//These methods take comparable time to produce a tree of comparable total volume
//the cache file is about 70% as big (ie fewer duplicated tris) so this is probably better
//on the other hand the teapot has a lot of overlap
#define OVERLAP_OK

//#include <iostream>

AABBTree::Resource::Resource(std::string file)
	: ResourceTy(file)
{
	auto p = Profile("build AABB");

	std::string cacheFile = file + ".aabb.cache";

	float volTotal = 0.f, overlapTotal = 0.f, imbTotal = 0.f;

	if (CacheIsFresh(file, cacheFile))
	{
		try
		{
			LoadCache(cacheFile);
			return;
		}
		catch (BlobFileException& ex)
		{
			std::cerr << ex.what() << '\n';
			//fall thru and load
		}
	}

	Mesh m = LoadMesh(file);

	static const int MAX_TRIS_PER_LEAF = 1;
	size_t height = static_cast<size_t>(
		std::ceilf(std::log2f(float(m.size() / MAX_TRIS_PER_LEAF))));

	tree = TreeTy::TopDown(height, Bound(m), m,
		[&](Mesh parent, const AlignedBox3f& cur)
	{
		if (std::isfinite(cur.volume()))
			volTotal += cur.volume();

		//Get longest axis
		Vector3f::Index longestAxis;
		cur.sizes().maxCoeff(&longestAxis);

		Vector3f meshCentroid = centroid(parent);

#ifdef OVERLAP_OK
		Mesh left, right;

		auto ax = longestAxis;

		while (true)
		{
			std::partition_copy(parent.begin(), parent.end(),
				std::back_inserter(left), std::back_inserter(right),
				[=](const Triangle& t)
			{
				//return t.row(ax).maxCoeff() < meshCentroid[ax];
				//produces much higher total volume, but more balanced tree
				return centroid(t)[ax] < meshCentroid[ax];
			});

			ax = (ax + 1) % 3;

			//If one side has the whole mesh, split it differently
			if (left.size() != 0 && right.size() != 0 //nondegenerate split
				|| ax == longestAxis) //no more axes to try
				break;

			//still axes to try
			left.clear(), right.clear();
		}

		if (left.size() + right.size() > 0)
			imbTotal += std::abs(float(left.size()) - float(right.size()))
				/ float(left.size() + right.size());

		auto lBound = Bound(left);
		auto rBound = Bound(right);

		float overlap = lBound.intersection(rBound).volume();
		if (std::isfinite(overlap))
			overlapTotal += overlap;

		return std::make_tuple(
			lBound, std::move(left),
			rBound, std::move(right));

#else
		AlignedBox3f left = cur, right = cur;
		left.max()[longestAxis] = centroid[longestAxis];
		right.min()[longestAxis] = centroid[longestAxis];

		//remove all non-intersecting elements
		Mesh mLeft = ConservativeChop(parent, left);
		Mesh mRight = ConservativeChop(std::move(parent), right);
		auto lBound = Bound(mLeft);
		auto rBound = Bound(mRight);

		return std::make_tuple(
			left.intersection(lBound), std::move(mLeft),
			right.intersection(rBound), std::move(mRight));
#endif
	},
		[](Mesh mLeft, const AlignedBox3f&)
	{
		return mLeft;
	}
	);
	std::cerr << "Total volume for " << file << ' ' << volTotal
		<< " overlapping " << overlapTotal
		<< " imbalace " << imbTotal / float(tree.end() - tree.begin()) << '\n';
	//SaveCache(cacheFile);
}

AABBTree::AABBTree(std::string file)
	: resource(Resource::FindOrMake(file))
{}

std::string AABBTree::Name() const
{
	return resource->Key();
}

const AABBTree::TreeTy& AABBTree::Tree() const
{
	return resource->tree;
}

//This would make a big difference if it weren't for the 100s of k's of
//mesh data after
//#define FIXED_AABB_CACHE
using AABBFixedType = std::uint8_t;

void AABBTree::Resource::SaveCache(std::string cacheFile)
{
	BlobOutFile file{ cacheFile, {'a','a','b','b'}, 1 };

	auto it = tree.cbegin();
	file.Write<BlobSizeType>(tree.Height());

	//write out the first box in full
	file.Write(*it);

	for (++it; it != tree.cend(); ++it)
	{
#ifdef FIXED_AABB_CACHE
		//write the rest in fixed point
		Eigen::Array3f scale = std::numeric_limits<AABBFixedType>::max()
			/ it.Parent()->sizes().array();
		Vector3f min = (it->min() - it.Parent()->min()).array() * scale;
		Vector3f max = (it->max() - it.Parent()->min()).array() * scale;
		
		file.Write(min.cast<AABBFixedType>().eval());
		file.Write(max.cast<AABBFixedType>().eval());
#else
		file.Write(*it);
#endif
	}

	auto leafit = tree.cbegin();
	for (; !leafit.Bottom(); leafit.ToLeft())
		;

	for (; leafit != tree.cend(); ++leafit)
	{
		//can't do this in the chop algorithm because the triangles stick
		//out of the boxes sometimes
#if defined(FIXED_AABB_CACHE) && defined(OVERLAP_OK)
		Eigen::Array3f scale = std::numeric_limits<AABBFixedType>::max()
			/ leafit->sizes().array();
		const auto& leaf = tree.Leaf(leafit);
		std::vector<Eigen::Array<AABBFixedType, 3, 3>> fixed(leaf.size());

		std::transform(leaf.begin(), leaf.end(), fixed.begin(), [&](const Triangle& t)
		{
			return ((t.colwise() - leafit->min().array())
				.colwise() * scale).cast<AABBFixedType>();
		});
		file.Write(fixed);
#else
		file.Write(tree.Leaf(leafit));
#endif
	}
}

void AABBTree::Resource::LoadCache(std::string cacheFile)
{
	BlobInFile file{ cacheFile, { 'a','a','b','b' }, 1 };

	auto it = tree.cbegin();
	auto height = file.Read<size_t, BlobSizeType>();
	auto root = file.Read<AlignedBox3f>();
	
	//relies on this running breadth-first
	tree = TreeTy::TopDown(height, root, 0,
		[&](int, const AlignedBox3f&)
	{
		auto left = file.Read<AlignedBox3f>();
		auto right = file.Read<AlignedBox3f>();
		return std::make_tuple(left, 0, right, 0);
	},
		[&](int, const AlignedBox3f&)
	{
		return file.ReadVector<Mesh::value_type, Mesh::allocator_type>();
	});
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

ShowAABB::ShowAABB(const AABBTree& aabb)
	: shaderProgram ("assets/color")
{
	const auto& tree = aabb.resource->tree;

	std::vector<AABBVert, AABBVert::Allocator> attribs;
	std::vector<LineInd> indices;

	auto it = tree.begin();
	//while (!it.Bottom()) ++it;

	for (; it != tree.end(); ++it)
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
