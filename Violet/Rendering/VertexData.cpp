#include "stdafx.h"
#include "VertexData.hpp"
#include "Geometry/Mesh.hpp"
#include "File/Wavefront.hpp"
#include "File/BlobFile.hpp"
#include "File/Filesystem.hpp"
#include "Utils/Profiling.hpp"
#include <iostream>

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
	//find it it memory
	resource = VertexDataResource::FindResource(file);
	if (resource) return;
	
	//find it cached
	if (CacheIsFresh(file, file + ".cache"))
	{
		try
		{
			resource = VertexDataResource::MakeShared(file);
			return;
		}
		catch (BlobFileException ex)
		{
			std::cerr << ex.what() << '\n';
			//fall through
		}
	}

	//load it
	if (IsWavefront(file))
		*this = WavefrontVertexData(file);
	else
		throw std::runtime_error("Unrecognized object file " + file);
}

std::string VertexData::Name() const
{
	return resource->Key();
}

template<>
const Schema AttribTraits<SimpleVert>::schema = {
	AttribProperties{"position", GL_FLOAT, false, 0,                 {3, 1}},
	AttribProperties{"texCoord", GL_FLOAT, false, 3 * sizeof(float), {2, 1}},
};

//just assume this
template<>
const Schema AttribTraits<Vector3f>::schema = {
	AttribProperties{"position", GL_FLOAT, false, 0, {3, 1}},
};

void VertexData::VertexDataResource::WriteCache(range<const char*> verts, range<const char*> inds)
{
	auto prof = Profile("vert cache");

	BlobOutFile cache(Key() + ".cache", { 'v','e','r','t' }, 1);

	cache.Write<std::int32_t>(vertexBufferStride);
	cache.Write<std::uint32_t>(mode);
	cache.Write<std::int32_t>(numVertecies);

	cache.Write<BlobSizeType>(vertexBufferSchema.size());
	for (const auto& props : vertexBufferSchema)
	{
		cache.Write(props.name);
		cache.Write<std::uint32_t>(props.glType);
		cache.Write<BlobSizeType>(props.offset);
		cache.Write              (props.integer);
		cache.Write<std::uint8_t>(props.dims.x());
		cache.Write<std::uint8_t>(props.dims.y());
		cache.Write<std::uint8_t>(0);
		cache.Write<BlobSizeType>(props.matrixStride);
	}

	cache.Write(verts);
	cache.Write(inds);
}

VertexData::VertexDataResource::VertexDataResource(const std::string& name)
	: Resource(name)
{
	auto prof = Profile("vert cache read");

	BlobInFile cache(name + ".cache", { 'v','e','r','t' }, 1);

	vertexBufferStride = cache.Read<std::int32_t>();
	mode = 				 cache.Read<std::uint32_t>();
	numVertecies = 		 cache.Read<std::int32_t>();

	size_t schemaSize = cache.Read<size_t, BlobSizeType>();
	vertexBufferSchema.reserve(schemaSize);
	while (vertexBufferSchema.size() < schemaSize)
	{
		AttribProperties props;

		props.name          = cache.ReadString();
		props.glType		= cache.Read<std::uint32_t>();
		props.offset		= cache.Read<size_t, BlobSizeType>();
		props.integer       = cache.ReadBool();
		props.dims.x()		= cache.Read<std::uint8_t>(); 
		props.dims.y()		= cache.Read<std::uint8_t>(); 
							  cache.Read<std::uint8_t>(); 
		props.matrixStride	= cache.Read<size_t, BlobSizeType>();

		vertexBufferSchema.push_back(props);
	}

	auto verts = cache.ReadVector<char>();
	vertexBuffer.Data(verts);
	auto inds = cache.ReadVector<char>();
	indexBuffer.Data(inds, IgnoreType);
}
