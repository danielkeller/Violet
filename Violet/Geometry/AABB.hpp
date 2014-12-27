#ifndef AABB_HPP
#define AABB_HPP
#include "Mesh.hpp"
#include "Containers/BinTree.hpp"

//#ifdef SOMETHING
#include "Rendering/VertexData.hpp"
#include "Rendering/Shader.hpp"
//#endif

class AABB
{
public:
	AABB(Mesh m);

private:
	using TreeTy = BinTree<Box, Mesh, Eigen::aligned_allocator<Box>>;
	void build(Mesh mLeft, Box cur, size_t depth, TreeTy::iterator it);
	TreeTy tree;
	friend struct ShowAABB;
};

struct ShowAABB
{
	VertexData vertData;
	ShaderProgram shaderProgram;
	ShowAABB(const AABB& aabb);
};

#endif
