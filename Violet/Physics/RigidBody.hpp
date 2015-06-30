#ifndef RIGID_BODY_HPP
#define RIGID_BODY_HPP

#include "Core/Component.hpp"
#include "Containers/l_unordered_map.hpp"
#include "Core/Time.hpp"

#include "Utils/DebugBoxes.hpp"

class Position;
class NarrowPhase;

//TODO: "physics mesh" component

//generalized coordinates, stores 3 linear components and 3 rotation components
using GenCoord = Eigen::Matrix<float, 6, 1>;

struct State
{
	Vector3f position;
	Quaternionf orientation;

	GenCoord momentum;
	//these shouldn't be here
	Eigen::DiagonalMatrix<float, 6> inverseIntertia;
	float mass, inertia;
};

class RigidBody : public Component
{
public:
	RigidBody(Position&, NarrowPhase&, RenderPasses&);
	void Add(Object o, float mass, float inertia);
	void PhysTick(Time::clock::duration simTime);
    
    bool& Debug() { return debug.enabled; }
    bool paused;

private:
	Position& position;
	NarrowPhase& narrowPhase;
	l_unordered_map<Object, State> data;

	void Load(const Persist&);
	void Unload(const Persist&);
	bool Has(Object) const;
	void Save(Object, Persist&) const;
    void Remove(Object);
    
    DebugBoxes debug;
};

MAKE_PERSIST_TRAITS(RigidBody, Object, float, float)

#endif
