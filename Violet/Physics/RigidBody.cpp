#include "stdafx.h"
#include "RigidBody.hpp"

#include "Position.hpp"
#include "NarrowPhase.hpp"
#include "File/Persist.hpp"

static const float simDt = std::chrono::duration<float>{ Time::dt }.count();

Vector3f QuatToRot(const Quaternionf& quat)
{
	Eigen::AngleAxisf rot{ quat };
	return rot.axis() * rot.angle();
}

Quaternionf RotToQuat(const Vector3f& vec)
{
	return vec.norm() == 0.f ? Quaternionf::Identity()
		: Quaternionf{ Eigen::AngleAxisf{ vec.norm(), vec.normalized() } };
}

struct Derivative
{
	Vector3f force, drag, velocity;
	Vector3f torque, angularVelocity;
};

Derivative zeroDeriv = { Vector3f::Zero(), Vector3f::Zero(),
Vector3f::Zero(), Vector3f::Zero(), Vector3f::Zero() };

void advance(State& s, const Derivative& d, float dt)
{
	s.momentum += d.force*dt;
	//to prevent drift, drag force must come after regular force has been
	//added to the momentum. This allows it to counteract forces that are
	//applied at this evaluation
	if (!s.momentum.isZero(.0001f)) //no drag without momentum
	{
		//Drag does no work, so F.ds <= 0
		//parallel component
		float parallelDrag = s.momentum.normalized().dot(d.drag);
		Vector3f dM = s.momentum.normalized()*std::min(0.f, //can't make it faster
			+ std::max(parallelDrag*dt, -s.momentum.norm())); //can't turn it around
		//perpendicular component
		dM += (d.drag - s.momentum.normalized()*parallelDrag)*dt;
		s.momentum += dM;
	}
	s.position += d.velocity*dt;
	s.angularMomentum += d.torque*dt;
	s.orientation += d.angularVelocity*dt;
}

//RK4 step
template<typename F>
Derivative step(const State& initial, float alpha, const Derivative& d,
	Time::clock::duration t, F forces)
{
	State s = initial;
	advance(s, d, simDt*alpha);

	Time::clock::duration newT = t +
		std::chrono::duration_cast<Time::clock::duration>(Time::dt*alpha);

	Derivative ret;
	std::tie(ret.force, ret.drag, ret.torque) = forces(s, newT);
	ret.velocity = s.momentum / s.mass;
	ret.angularVelocity = s.angularMomentum / s.inertia;
	return ret;
}

//RK4 integrate
template<typename F>
void integrate(State& state, Time::clock::duration t, F forces)
{
	Derivative a, b, c, d;
	a = step(state, 0.f, zeroDeriv, t, forces);
	b = step(state, 5.f, a, t, forces);
	c = step(state, 5.f, b, t, forces);
	d = step(state, 1.f, c, t, forces);

	Derivative final;
#define RKINTERP(val) final.val = 1.f / 6.f * (a.val + 2.f*(b.val + c.val) + d.val)
	RKINTERP(force);
	RKINTERP(drag);
	RKINTERP(velocity);
	RKINTERP(torque);
	RKINTERP(angularVelocity);
	advance(state, final, simDt);
}

#include <iostream>

void RigidBody::PhysTick(Time::clock::duration simTime)
{
	static const Vector3f gravity = Vector3f{ 0, 0, -9.8f };

	for (auto& obj : data)
	{
		State& state = obj.second;

		//pull this in in case it changed
		Transform xfrm = position[obj.first].get();
		state.position = xfrm.pos;
		state.orientation = QuatToRot(xfrm.rot);

		Vector3f normalForce = Vector3f::Zero();

		auto contacts = narrowPhase.QueryAll(obj.first);
		size_t ncontacts = contacts.size();

		if (ncontacts)
		{
			//Vector3f tangentMometum = state.momentum - state.momentum.dot()
			Eigen::MatrixXf::Index appliedForceDims = 3;

			//rows are dimensions of applied forces
			Eigen::MatrixXf F = Eigen::MatrixXf::Zero(appliedForceDims, ncontacts);
			Eigen::VectorXf b = Eigen::VectorXf::Zero(appliedForceDims);
			b.block<3, 1>(0, 0) = -gravity;

			//can this contact oppose the force while remaining positive?
			for (size_t j = 0; j < ncontacts; ++j)
				if (contacts[j].bNormal.dot(-gravity) > 0.f)
					F.block<3, 1>(0, j) = contacts[j].bNormal;

			//Find the least-squares solution, ie, the smallest force that will counteract
			//the applied force. Note that this minimizes both the norm of the force vector
			//among redundant contacts (ie, spreads the load evenly) and minimizes the
			//residual (ie, counteracts the force as much as possible without overdoing it).
			//Right now the SVD is the only decomposition I've found that correctly handles
			//singular least squares systems. This isn't great because the SVD is very slow,
			//so hopefully the QR can be made to work.
			Eigen::JacobiSVD<Eigen::MatrixXf> decomp{ F, Eigen::ComputeThinU | Eigen::ComputeThinV };
			Eigen::VectorXf N = decomp.solve(b);
			
			//std::cerr << F << " *\n\n" << N << " =\n\n" << b << "\n\n\n";

			for (size_t j = 0; j < ncontacts; ++j)
			{
				assert(N[j] > -.01f && "Negative normal force");
				normalForce += contacts[j].bNormal * N[j];
			}
		}

		for (size_t j = 0; j < ncontacts; ++j)
		{
			Vector3f contactNormal = contacts[j].bNormal;
			float badMomentum = -state.momentum.dot(contactNormal);
			if (badMomentum <= 0.f)
				continue;
			//should this be in the integrator?
			Vector3f dM = contactNormal*badMomentum;
			state.momentum += dM*(1.f + .8f); //restitution
			state.position += dM / state.mass * simDt;
		}

		integrate(state, simTime,
			[=](const State& state, Time::clock::duration t)
		{
			return std::make_tuple(gravity, normalForce, Vector3f{ 0,0,0 });
		});

		//std::cerr << state.momentum << "\n\n";

		xfrm.pos = state.position;
		xfrm.rot = RotToQuat(state.orientation);
		position[obj.first].set(xfrm);
	}
}

RigidBody::RigidBody(Position& position, NarrowPhase& narrowPhase)
	: position(position), narrowPhase(narrowPhase)
{
}

void RigidBody::Load(const Persist& persist)
{
	for (auto& dat : persist.GetAll<RigidBody>())
		Add(std::get<0>(dat), std::get<1>(dat), std::get<2>(dat));
}

void RigidBody::Save(Object obj, Persist& persist) const
{
	//TODO: this is dumb
	if (Has(obj))
		persist.Set<RigidBody>(obj, data.at(obj).mass, data.at(obj).inertia);
	else
		persist.Delete<RigidBody>(obj);
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

MAP_COMPONENT_BOILERPLATE(RigidBody, data);

template<>
const char* PersistSchema<RigidBody>::name = "rigidbody";
template<>
Columns PersistSchema<RigidBody>::cols = { "object", "mass", "inertia" };
