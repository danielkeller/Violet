#ifndef OBB_HPP
#define OBB_HPP

#include "Containers/BinTree.hpp"
#include "Mesh.hpp"
#include "Shapes.hpp"

struct ResourcePersistTag;

class OBBTree
{
	struct Resource;
public:
	OBBTree(std::string file);
	std::string Name() const;

	using TreeTy = BinTree<variant<OBB, Triangle>>;
	const TreeTy& Tree() const;

	using PersistCategory = ResourcePersistTag;

private:
	std::shared_ptr<Resource> resource;
	friend struct ShowAABB;
};

#endif
