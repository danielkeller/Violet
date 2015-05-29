#ifndef NARROW_PHASE_HPP
#define NARROW_PHASE_HPP

#include "Core/Component.hpp"
#include "Geometry/AABB.hpp"
#include <unordered_map>

#include "Rendering/Render.hpp"

class Position;
class RenderPasses;

class NarrowPhase : public Component
{
public:
	NarrowPhase(Position&, RenderPasses&);
	void Add(Object obj, std::string mesh);
	std::vector<Vector3f> Query(Object a, Object b);

private:
	void Load(const Persist&);
	void Unload(const Persist&);
	bool Has(Object) const;
	void Save(Object, Persist&) const;
	void Remove(Object);

	Position& position;
	std::unordered_map<Object, AABB> data;

	Object debugObj;
	Material dbgMat;
	VAO dbgVao;

	struct DebugInst
	{
		Matrix4f loc;
		Vector3f color;
	};

	BufferObject<DebugInst, GL_ARRAY_BUFFER, GL_STREAM_DRAW> instances;
};

MAKE_PERSIST_TRAITS(NarrowPhase, Object, AABB)

#endif
