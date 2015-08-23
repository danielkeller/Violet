#ifndef NARROW_PHASE_HPP
#define NARROW_PHASE_HPP

#include "Core/Component.hpp"
#include "Geometry/OBB.hpp"
#include <unordered_map>
#include "Containers/l_unordered_map.hpp"

#include "Utils/DebugBoxes.hpp"

#include "Geometry/AABB.hpp"

class Position;
class RenderPasses;

struct Contact
{
    Object a, b;
	Vector3f point;
	//contact normals in world coordinates
	Vector3f aNormal, bNormal;
};

class Collision : public Component
{
public:
	Collision(Position&, RenderPasses&);
	void Add(Object obj, OBBTree mesh);
    const std::vector<Contact>& Contacts() const {return result;}
    void PhysTick();
    
    bool& Debug() { return debug.enabled; }
    
    using TreeTy = OBBTree;

private:
	void Load(const Persist&);
	void Unload(const Persist&);
	bool Has(Object) const;
	void Save(Object, Persist&) const;
	void Remove(Object);
    
    //Broad Phase:
    AABBTree<Object> broadTree;
    l_unordered_map<Object, AABBTree<Object>::iterator> broadLeaves;
    
    //Narrow Phase:
	std::unordered_map<Object, TreeTy> data;
    
    using Iter = TreeTy::TreeTy::const_iterator;
    using IterPair = std::pair<Iter, Iter>;
    std::vector<IterPair> nodesToCheck;
    
    void NarrowPhase(Object a, Object b);
    
    //Other:
    Position& position;
    Box3 Bound(Object obj) const;
    std::vector<Contact> result;
    mutable DebugBoxes debug;
};

MAKE_PERSIST_TRAITS(Collision, Object, Collision::TreeTy)

#endif
