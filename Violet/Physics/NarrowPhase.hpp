#ifndef NARROW_PHASE_HPP
#define NARROW_PHASE_HPP

#include "Core/Component.hpp"
#include "Geometry/OBB.hpp"
#include <unordered_map>

#include "Utils/DebugBoxes.hpp"

class Position;
class RenderPasses;

struct Contact
{
	Vector3f point;
	//contact normals in world coordinates
	Vector3f aNormal;
	Vector3f bNormal;
};

class NarrowPhase : public Component
{
public:
	NarrowPhase(Position&, RenderPasses&);
	void Add(Object obj, std::string mesh);
	std::vector<Contact> Query(Object a, Object b) const;
	//FIXME: broadphase
	std::vector<Contact> QueryAll(Object a) const;

    using TreeTy = OBBTree;
    
    bool& Debug() { return debug.enabled; }

private:
	void Load(const Persist&);
	void Unload(const Persist&);
	bool Has(Object) const;
	void Save(Object, Persist&) const;
	void Remove(Object);

	Position& position;
	std::unordered_map<Object, TreeTy> data;
    
    using Iter = NarrowPhase::TreeTy::TreeTy::const_iterator;
    using IterPair = std::pair<Iter, Iter>;
    
    mutable DebugBoxes debug;
    mutable std::vector<IterPair> nodesToCheck;
};

MAKE_PERSIST_TRAITS(NarrowPhase, Object, NarrowPhase::TreeTy)

#endif
