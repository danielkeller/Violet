#include "stdafx.h"
#include "VertexData.hpp"
#include "Geometry/Mesh.hpp"

struct SimpleVert
{
    Vector3f position;
    Vector2f texCoord;
};

std::vector<SimpleVert> boxVerts = {
    {{0, 0, 0}, {0, 0}},
    {{1, 0, 0}, {1, 0}},
    {{0, 1, 0}, {0, 1}},
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

template<>
const Schema AttribTraits<SimpleVert>::schema = {
    {"position", 3, GL_FLOAT, 0,                 1},
    {"texCoord", 2, GL_FLOAT, 3 * sizeof(float), 1},
};
