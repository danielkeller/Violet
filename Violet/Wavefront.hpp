#include "Rendering/VertexData.hpp"
#include "Rendering/Shader.hpp"
#include "Geometry/Mesh.hpp"
#include <tuple>

struct Wavefront
{
	VertexData vertexData;
	Mesh mesh;
	ShaderProgram shaderProgram;
	Wavefront(std::string filename);
};