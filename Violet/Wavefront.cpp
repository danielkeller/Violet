#include "stdafx.h"
#include "Wavefront.hpp"
#include "Geometry/Mesh.hpp"
#include "Rendering/VertexData.hpp"
#include "Profiling.hpp"

#include <fstream>

bool IsWavefront(const std::string filename)
{
	return ends_with(filename, ".obj");
}

struct WavefrontVert
{
	//Prevent alignment issues from causing asserts
	using Allocator = Eigen::aligned_allocator<WavefrontVert>;

	Vector3f pos;
	Vector3f norm;
	Eigen::Vector2f uv;
};

template<>
const Schema AttribTraits<WavefrontVert>::schema = {
    {"position", GL_FLOAT, false, 0,                 {3, 1}},
    {"normal",   GL_FLOAT, false, 3 * sizeof(float), {3, 1}},
    {"texCoord", GL_FLOAT, false, 6 * sizeof(float), {2, 1}},
};

struct Wavefront
{
	vectorVector3f verts;
	vectorVector3f norms;
	std::vector<Vector2f, Eigen::aligned_allocator<Vector2f>> uvs;
	std::vector<TriInd> indices;

	Wavefront(std::string filename);
};

Wavefront::Wavefront(std::string filename)
{
	auto p = Profile::Profile("wavefront load");

	std::ifstream obj(filename);
	if (obj.fail())
		throw std::runtime_error("Cannot open object file '" + filename + "'");
        
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
		else if (letter == "vn")
		{
			obj >> x >> y >> z;
			norms.push_back(Vector3f(x, y, z));
		}
		else if (letter == "vt")
		{
			obj >> x >> y;
			uvs.push_back(Vector2f(1-x, 1-y));
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
}

VertexData WavefrontVertexData(std::string filename)
{
	Wavefront w(filename);

	auto vert_it = w.verts.begin();
	auto norm_it = w.norms.begin();
	auto uv_it = w.uvs.begin();

	std::vector<WavefrontVert, WavefrontVert::Allocator> attribs;

	for (; vert_it != w.verts.end(); ++vert_it)
		attribs.emplace_back(WavefrontVert{ *vert_it,
		norm_it < w.norms.end() ? Vector3f{ *norm_it++ } : Vector3f::Zero(),
		uv_it < w.uvs.end() ? Vector2f{ *uv_it++ } : Vector2f::Zero() });

	return{ filename, attribs, w.indices };
}

Mesh WavefrontMesh(std::string filename)
{
	Wavefront w(filename);
	return{ filename, w.verts, w.indices };
}