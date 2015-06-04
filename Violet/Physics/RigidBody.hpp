#ifndef RIGID_BODY_HPP
#define RIGID_BODY_HPP

#include "Core/Object.hpp"
#include "Containers/l_unordered_map.hpp"
#include "Core/Time.hpp"

class Position;
class NarrowPhase;

//TODO: "physics mesh" component

struct State
{
	State();
	Vector3f position, momentum;
	float mass;

	Vector3f orientation, angularMomentum;
	float inertia;
};

class RigidBody
{
public:
	RigidBody(Position&, NarrowPhase&);
	void Add(Object o, float mass, float inertia);
	void PhysTick(Time::clock::duration simTime);

private:
	Position& position;
	NarrowPhase& narrowPhase;
	l_unordered_map<Object, State> data;
};

#endif
