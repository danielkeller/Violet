#include "stdafx.h"
#include "Wavefront.hpp"
#include "Geometry/Mesh.hpp"

#include <fstream>

struct WavefrontVert
{
	//Prevent alignment issues from causing asserts
	using Allocator = Eigen::aligned_allocator<WavefrontVert>;

	Vector3f pos;
	Vector3f norm;
	Eigen::Vector2f uv;
	static const Schema schema;
};

const Schema WavefrontVert::schema = {
	{"position", 3, GL_FLOAT, 0,                 1},
	{"normal",   3, GL_FLOAT, 3 * sizeof(float), 1},
	{"texCoord", 2, GL_FLOAT, 6 * sizeof(float), 1},
};

Wavefront::Wavefront(std::string filename)
	//shader appropriate for wavefront objects
	: shaderProgram("assets/simple")
{
	shaderProgram.TextureOrder({ "tex" });

	std::shared_ptr<VertexData_detail::VertexDataResource> vertptr
		= VertexData_detail::VertexDataResource::FindResource(filename);
	std::shared_ptr<Mesh::MeshResource> meshptr = Mesh::MeshResource::FindResource(filename);
	
	if (vertptr && meshptr)
	{
		vertexData = vertptr;
		mesh = meshptr;
		return;
	}

	std::ifstream obj(filename);
	if (obj.fail())
		throw std::runtime_error("Cannot open object file '" + filename + "'");

	vectorVector3f verts;
	vectorVector3f norms;
	std::vector<Vector2f, Eigen::aligned_allocator<Vector2f>> uvs;
	std::vector<TriInd> indices;
        
    std::string letter;
    GLfloat x, y, z;
    GLint a, b, c;

    while(obj)
    {
        obj >> letter;
        if (letter == "v")
        {
            obj >> x >> y >> z;
            verts.push_back(Vector3f(x, y, z));
		}
		else if (!vertptr && letter == "vn")
		{
			obj >> x >> y >> z;
			norms.push_back(Vector3f(x, y, z));
		}
		else if (!vertptr && letter == "vt")
		{
			obj >> x >> y;
			uvs.push_back(Vector2f(1-x, y));
		}
        else if (letter == "f")
        {
			//Obj files can have this notation like
			//f 2863/2863/2863 2864/2864/2864 2965/2965/2965
			//where (i guess) you can specify different indices for different attributes.
			obj >> a;
			obj.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
			obj >> b;
			obj.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
			obj >> c;
			obj.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            //whose brilliant idea was it to make this 1-based
			indices.push_back({ a - 1, b - 1, c - 1 });
        }
		else //ignore everything else
			obj.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

	if (!vertptr)
	{
		auto vert_it = verts.begin();
		auto norm_it = norms.begin();
		auto uv_it = uvs.begin();

		std::vector<WavefrontVert, WavefrontVert::Allocator> attribs;

		for (; vert_it != verts.end(); ++vert_it)
			attribs.emplace_back(WavefrontVert{ *vert_it,
				norm_it < norms.end() ? Vector3f{ *norm_it++ } : Vector3f::Zero(),
				uv_it < uvs.end() ? Vector2f{ *uv_it++ } : Vector2f::Zero() });

		vertexData = VertexData_detail::VertexDataResource::MakeShared(
			filename, attribs, indices);
	}

	if (!meshptr)
		mesh = Mesh::MeshResource::MakeShared(filename, verts, indices);
}
