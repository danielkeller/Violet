#include "stdafx.h"
#include "VertexData.hpp"
#include "Geometry/Mesh.hpp"
#include "File/Wavefront.hpp"

struct SimpleVert
{
    Vector3f position;
    Vector2f texCoord;
};

std::vector<SimpleVert> boxVerts = {
    {{-1, -1, 0}, {0, 0}},
    {{1, -1, 0}, {1, 0}},
    {{-1, 1, 0}, {0, 1}},
    {{1, 1, 0}, {1, 1}}
};
std::vector<TriInd> boxInds = {
    {0, 1, 2},
    {1, 3, 2}
};

VertexData::VertexData(UnitBoxT)
{
    resource = VertexDataResource::FindResource("UnitBox");
    if (!resource)
        resource = VertexDataResource::MakeShared("UnitBox", boxVerts, boxInds);
}

VertexData::VertexData(const std::string& file)
{
	resource = VertexDataResource::FindResource(file);
	if (!resource)
	{
		if (IsWavefront(file))
			*this = WavefrontVertexData(file);
		else
			throw std::runtime_error("Unrecognized object file " + file);
	}
}

std::string VertexData::Name() const
{
	return resource->Key();
}

template<>
const Schema AttribTraits<SimpleVert>::schema = {
    {"position", GL_FLOAT, false, 0,                 {3, 1}},
    {"texCoord", GL_FLOAT, false, 3 * sizeof(float), {2, 1}},
};

//just assume this
template<>
const Schema AttribTraits<Vector3f>::schema = {
    {"position", GL_FLOAT, false, 0, {3, 1}},
};
