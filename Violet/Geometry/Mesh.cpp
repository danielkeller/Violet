#include "stdafx.h"
#include "Mesh.hpp"
#include "Collide.hpp"
#include <functional>
#include <iostream>

using namespace std::placeholders;

MeshIter Mesh::begin()
{
	if (!submesh.size())
		submesh = resource->indices;

	return{ submesh.begin(), resource->points };
}

MeshIter Mesh::end()
{
	if (!submesh.size())
		submesh = resource->indices;

	return{ submesh.end(), resource->points };
}

ConstMeshIter Mesh::begin() const
{
	if (submesh.size())
		return{ submesh.begin(), resource->points };
	else
		return{ resource->indices.begin(), resource->points };
}

ConstMeshIter Mesh::end() const
{
	if (submesh.size())
		return{ submesh.end(), resource->points };
	else
		return{ resource->indices.end(), resource->points };
}

void Mesh::Chop(Box box)
{
	if (!submesh.size())
		submesh = resource->indices;
	submesh.erase(std::remove_if(submesh.begin(), submesh.end(),
		[&](const TriInd& tri) {
		return !Intersects(box, Triangle{
			resource->points[tri.a],
			resource->points[tri.b],
			resource->points[tri.c] });
		}),
		submesh.end());
}

void Mesh::PrintDataSize()
{
	std::cout << resource->points.size() * sizeof(Vector3f) << "b points "
		<< resource->indices.size() * sizeof(TriInd) << "b inds "
		<< (resource->points.size() * sizeof(Vector3f) +
		resource->indices.size() * sizeof(TriInd)) / 1024 << "kb total\n";
}

Box Mesh::bound() const
{
	float maxflt = std::numeric_limits<float>::max();
	float minflt = std::numeric_limits<float>::min();
	Vector3f min{ maxflt, maxflt, maxflt };
	Vector3f max{ minflt, minflt, minflt };

	for (const auto& tri : *this)
	{
		min = min.cwiseMin(tri.Triangle().q);
		min = min.cwiseMin(tri.Triangle().r);
		min = min.cwiseMin(tri.Triangle().s);
		max = max.cwiseMax(tri.Triangle().q);
		max = max.cwiseMax(tri.Triangle().r);
		max = max.cwiseMax(tri.Triangle().s);
	}
	return{ min, max };
}