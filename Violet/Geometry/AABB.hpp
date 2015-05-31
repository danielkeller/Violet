#ifndef AABB_HPP
#define AABB_HPP

#include "Containers/BinTree.hpp"
#include "Mesh.hpp"

//#ifdef SOMETHING
#include "Rendering/VertexData.hpp"
#include "Rendering/Shader.hpp"
//#endif

class AABBTree
{
	struct Resource;
public:
	AABBTree(std::string file);
	std::string Name() const;

	using TreeTy = BinTree<AlignedBox3f, Mesh>;
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
	ShowAABB(const AABBTree& aabb);
};

#endif
