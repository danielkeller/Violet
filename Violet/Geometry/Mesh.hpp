#ifndef MESH_HPP
#define MESH_HPP

#include "Shapes.hpp"

class VAO;
class ShaderProgram;

//These should go in a general geometry loading header*
//{
struct LineInd
{
	GLint a, b;
	static const GLenum mode = GL_LINES;
};

struct TriInd
{
	GLint a, b, c;
	static const GLenum mode = GL_TRIANGLES;
};
//}

//This is faster to deal with than any fancy point-index thing
using Mesh = std::vector<Triangle, Eigen::aligned_allocator<Triangle>>;

Mesh LoadMesh(std::string file);
//*as should this
Mesh MakeMesh(std::vector<Vector3f> verts, std::vector<TriInd> inds);

AlignedBox3f Bound(const Mesh&);

//Remove all triangles not touching box
Mesh ConservativeChop(Mesh, AlignedBox3f box);

#endif
