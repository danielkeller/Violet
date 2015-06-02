#include "stdafx.h"
#include "RigidBody.hpp"

#include "Position.hpp"

static const float simDt = std::chrono::duration<float>{ Time::dt }.count();

Vector3f QuatToRot(const Quaternionf& quat)
{
	Eigen::AngleAxisf rot{ quat };
	return rot.axis() * rot.angle();
}

Quaternionf RotToQuat(const Vector3f& vec)
{
	return Quaternionf{ Eigen::AngleAxisf{ vec.norm(), vec.normalized() } };
}

struct Derivative
{
	Vector3f velocity, force;
	Vector3f angularVelocity, torque;
};

void advance(State& s, const Derivative& d, float dt)
{
	s.momentum += d.force*dt;
	s.position += d.velocity*dt;
	s.angularMomentum += d.torque*dt;
	s.orientation += d.angularVelocity*dt;
}

//RK4 step
template<typename F, typename T>
Derivative step(const State& initial, float alpha, const Derivative& d,
	Time::clock::duration t, F force, T torque)
{
	State s = initial;
	advance(s, d, simDt*alpha);

	Time::clock::duration newT = t +
		std::chrono::duration_cast<Time::clock::duration>(Time::dt*alpha);

	Derivative ret;
	ret.force = force(s, newT);
	ret.velocity = s.momentum / s.mass;
	ret.torque = torque(s, newT);
	ret.angularVelocity = s.angularMomentum / s.inertia;
	return ret;
}

//RK4 integrate
template<typename F, typename T>
void integrate(State& state, Time::clock::duration t, F force, T torque)
{
	Derivative a, b, c, d;
	a = step(state, 0.f, { Vector3f::Zero(), Vector3f::Zero() }, t, force, torque);
	b = step(state, 5.f, a, t, force, torque);
	c = step(state, 5.f, b, t, force, torque);
	d = step(state, 1.f, c, t, force, torque);

	Derivative final;
	final.force = 1.f / 6.f * (a.force + 2.f*(b.force + c.force) + d.force);
	final.velocity = 1.f / 6.f * (a.velocity + 2.f*(b.velocity + c.velocity) + d.velocity);
	final.angularVelocity = 1.f / 6.f * (a.angularVelocity +
		2.f*(b.angularVelocity + c.angularVelocity) + d.angularVelocity);
	final.torque = 1.f / 6.f * (a.torque + 2.f*(b.torque + c.torque) + d.torque);
	advance(state, final, simDt);
}

void RigidBody::PhysTick(Time::clock::duration simTime)
{
	for (auto& obj : data)
	{
		State& state = obj.second;
		//pull this in in case it changed
		Transform xfrm = position[obj.first].get();
		state.position = xfrm.pos;
		state.orientation = QuatToRot(xfrm.rot);

		integrate(state, simTime,
			[](const State& state, Time::clock::duration t)
		{
			return Vector3f{ 0, 0, -3.f * state.position.z() };
		},
			[](const State& state, Time::clock::duration t)
		{
			return Vector3f{ 0, 0, 1 } - state.angularMomentum * .1f;
		});

		xfrm.pos = state.position;
		xfrm.rot = RotToQuat(state.orientation);
		position[obj.first].set(xfrm);
	}
}

RigidBody::RigidBody(Position& position)
	: position(position)
{
}

void RigidBody::Add(Object o, float mass, float inertia)
{
	State init;
	init.momentum = Vector3f::Zero();
	init.mass = mass;
	init.angularMomentum = Vector3f::Zero();
	init.inertia = inertia;

	data.try_emplace(o, init);
}

State::State()
{}
