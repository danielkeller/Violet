#ifndef AABB_HPP
#define AABB_HPP

#include "Containers/BinTree.hpp"
#include "Mesh.hpp"

//#ifdef SOMETHING
#include "Rendering/VertexData.hpp"
#include "Rendering/Shader.hpp"
//#endif

class AABB
{
	struct Resource;
public:
	AABB(std::string file);
	std::string Name() const;

	using TreeTy = BinTree<AlignedBox3f, Mesh, Eigen::aligned_allocator<AlignedBox3f>>;
	const TreeTy& Tree() const;

	using PersistCategory = ResourcePersistTag;

private:
	std::shared_ptr<Resource> resource;
	friend struct ShowAABB;
};

struct ShowAABB
{
	VertexData vertData;
	ShaderProgram shaderProgram;
	ShowAABB(const AABB& aabb);
};

#endif
