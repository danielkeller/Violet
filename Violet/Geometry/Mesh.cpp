#include "stdafx.h"
#include "Mesh.hpp"
#include "Collide.hpp"
#include "File/Wavefront.hpp"
#include <functional>
#include <iostream>

using namespace std::placeholders;

Mesh LoadMesh(std::string file)
{
	if (IsWavefront(file))
		return WavefrontMesh(file);
	else
		throw std::runtime_error("Unrecognized object file " + file);
}

Mesh MakeMesh(Verts vert, std::vector<TriInd> inds)
{
	Mesh ret;
	ret.resize(inds.size());
	auto it = ret.begin();

	for (const auto& ind : inds)
		*it++ << vert[ind.a], vert[ind.b], vert[ind.c];

	return ret;
}

Mesh ApproxChop(Mesh m, AlignedBox3f box)
{
	m.erase(std::remove_if(m.begin(), m.end(),
		[&](const Triangle& tri) {
		return !ApproxIntersects(box, tri);
		}),
		m.end());
	return m;
}

AlignedBox3f Bound(const Mesh& m)
{
	float maxflt = std::numeric_limits<float>::max();
	Eigen::Array3f min{ maxflt, maxflt, maxflt };
	Eigen::Array3f max = -min;

	for (const auto& tri : m)
	{
		min = min.cwiseMin(tri.rowwise().minCoeff());
		max = max.cwiseMax(tri.rowwise().maxCoeff());
	}

	return{ min, max };
}