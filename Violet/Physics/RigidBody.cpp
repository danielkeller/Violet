#include "stdafx.h"
#include "RigidBody.hpp"

#include "Position.hpp"
#include "NarrowPhase.hpp"

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

//TODO: do we really care about force?
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
#define RKINTERP(val) final.val = 1.f / 6.f * (a.val + 2.f*(b.val + c.val) + d.val)
	RKINTERP(force);
	RKINTERP(velocity);
	RKINTERP(angularVelocity);
	RKINTERP(torque);
	advance(state, final, simDt);
}

#include <iostream>

void RigidBody::PhysTick(Time::clock::duration simTime)
{
	for (auto& obj : data)
	{
		State& state = obj.second;

		//pull this in in case it changed
		Transform xfrm = position[obj.first].get();
		state.position = xfrm.pos;
		state.orientation = QuatToRot(xfrm.rot);

		//apply impulses
		//if (state.compression.isZero(.01f)) //use a generous epsilon to flush to zero
		//	state.compression = Vector3f::Zero();

		state.momentum += state.compression;
		state.compression = Vector3f::Zero();

		Vector3f normalForce = Vector3f::Zero();

		auto contacts = narrowPhase.QueryAll(obj.first);
		size_t ncontacts = contacts.size();

		if (ncontacts)
		{
			Eigen::MatrixXf F(1, ncontacts); //rows are applied forces
			Eigen::VectorXf b = Eigen::VectorXf::Zero(1);
			b[0] = 10.f;

			for (size_t j = 0; j < ncontacts; ++j)
			{
				//degree of opposition to applied force
				float proj = -contacts[j].bNormal.dot(Vector3f{ 0, 0, -1.f });
				F(0, j) = std::max(proj, 0.f);
			}

			Eigen::HouseholderQR<Eigen::MatrixXf> decomp{ F.transpose() };
			Eigen::VectorXf N = decomp.householderQ() * Eigen::MatrixXf::Identity(ncontacts, 1) *
				decomp.matrixQR().topLeftCorner(1, 1).triangularView<Eigen::Upper>().transpose().solve(b);

			for (size_t j = 0; j < ncontacts; ++j)
				normalForce += contacts[j].bNormal * N[j];
		}

		integrate(state, simTime,
			[=](const State& state, Time::clock::duration t)
		{
			return normalForce + Vector3f{ 0, 0, -10.f };
		},
			[](const State& state, Time::clock::duration t)
		{
			return Vector3f{ 0, 0, 0 };
		});

		for (size_t j = 0; j < ncontacts; ++j)
		{
			Vector3f contactNormal = contacts[j].bNormal;
			float badMomentum = -state.momentum.dot(contactNormal);
			if (badMomentum <= 0.f)
				continue;
			//should this be in the integrator?
			Vector3f dM = contactNormal*badMomentum;
			state.position += dM / state.mass * simDt;
			//state.compression += dM;
			state.momentum += dM;
		}
		state.compression *= .8f; //restitution

		xfrm.pos = state.position;
		xfrm.rot = RotToQuat(state.orientation);
		position[obj.first].set(xfrm);
	}
}

RigidBody::RigidBody(Position& position, NarrowPhase& narrowPhase)
	: position(position), narrowPhase(narrowPhase)
{
}

void RigidBody::Add(Object o, float mass, float inertia)
{
	State init;
	init.momentum = Vector3f::Zero();
	init.mass = mass;
	init.angularMomentum = Vector3f::Zero();
	init.inertia = inertia;

	init.compression = Vector3f::Zero();

	data.try_emplace(o, init);
}

State::State()
{}
