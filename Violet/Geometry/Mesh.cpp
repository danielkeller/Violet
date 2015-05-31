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

Mesh MakeMesh(std::vector<Vector3f> vert, std::vector<TriInd> inds)
{
	Mesh ret;
	ret.resize(inds.size());
	auto it = ret.begin();

	for (const auto& ind : inds)
		*it++ << vert[ind.a], vert[ind.b], vert[ind.c];

	return ret;
}

Mesh ConservativeChop(Mesh m, AlignedBox3f box)
{
	m.erase(std::remove_if(m.begin(), m.end(),
		[&](const Triangle& tri) {
		return !ConservativeIntersects(box, tri);
		}),
		m.end());
	return m;
}
