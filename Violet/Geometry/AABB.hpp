#ifndef AABB_HPP
#define AABB_HPP
#include "Mesh.hpp"
#include "BinTree.hpp"

//#ifdef SOMETHING

#include <tuple>
#include "Rendering/VAO.hpp"
#include "Rendering/Shader.hpp"

//#endif

class AABB
{
public:
	AABB(Mesh m);
	std::tuple<VAO, ShaderProgram> Show();
private:
	using TreeTy = BinTree<Box, Mesh, Eigen::aligned_allocator<Box>>;
	void build(Mesh mLeft, Box cur, size_t depth, TreeTy::iterator it);
	TreeTy tree;
};

#endif