#ifndef NARROW_PHASE_HPP
#define NARROW_PHASE_HPP

#include "Core/Component.hpp"
#include "Geometry/AABB.hpp"
#include "Geometry/OBB.hpp"
#include <unordered_map>

#include "Rendering/Render.hpp"

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
    
    //FIXME pending SO question
    struct DebugInst
    {
        Matrix4f loc;
        Vector3f color;
    };

private:
	void Load(const Persist&);
	void Unload(const Persist&);
	bool Has(Object) const;
	void Save(Object, Persist&) const;
	void Remove(Object);

	Position& position;
	std::unordered_map<Object, TreeTy> data;

	Object debugObj;
	Material dbgMat;
	mutable VAO dbgVao;
	mutable std::vector<DebugInst> insts;
	mutable BufferObject<DebugInst, GL_ARRAY_BUFFER, GL_STREAM_DRAW> instances;
};

MAKE_PERSIST_TRAITS(NarrowPhase, Object, NarrowPhase::TreeTy)

#endif
