#include "stdafx.h"
#include "RigidBody.hpp"

#include "Position.hpp"
#include "NarrowPhase.hpp"
#include "File/Persist.hpp"

#include <iostream>

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

//The generalized contact normal is a translation and rotation that moves
//the contact point perpendicularly away from the surface. In the case of
//a collision between two free objects, it's the translation and rotation
//of both that moves the objects away from each other.
//TODO: two-object case
GenCoord GenContact(const Contact& c, const Vector3f& objPos)
{
	//the derivative along n of the rotation of a vector v w.r.t the rotataton
	//angle theta, evaluated at theta=0, is ||v|| sin(alpha), where alpha is the
	//angle between v and n. Conveniently enough, the direction of the surface
	//normal in rotation coordinates is in cross(v, n), and n is already normalized.
	return (GenCoord() << c.bNormal, c.bNormal.cross(objPos - c.point)
		).finished().normalized();
}

struct Derivative
{
	GenCoord force, drag, velocity;
};

Derivative zeroDeriv = { GenCoord::Zero(), GenCoord::Zero(), GenCoord::Zero() };

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
		GenCoord dM = s.momentum.normalized()*std::min(0.f, //can't make it faster
			+ std::max(parallelDrag*dt, -s.momentum.norm())); //can't turn it around
		//perpendicular component
		dM += (d.drag - s.momentum.normalized()*parallelDrag)*dt;
		s.momentum += dM;
	}
	s.position += d.velocity*dt;
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
	std::tie(ret.force, ret.drag) = forces(s, newT);
	ret.velocity = s.inverseIntertia * s.momentum;
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
	advance(state, final, simDt);
}

void RigidBody::PhysTick(Time::clock::duration simTime)
{
    if (paused)
        return;
    
    debug.Begin();
	using Index = Eigen::MatrixXf::Index;
	//number of rows, degrees of freeedom
	Index m = 6;

	for (auto& obj : data)
	{
		State& state = obj.second;

		//wrap the rotation vector around to prevent loss of significance
		auto rot = state.position.block<3, 1>(3, 0);
		float angle = rot.norm() / PI_F;
		if (angle > 1.f)
			rot *= (1.f - 2.f / angle);

		auto contacts = narrowPhase.QueryAll(obj.first);

		//see definition of GenContact
		std::vector<GenCoord> genContacts(contacts.size());
		std::transform(contacts.begin(), contacts.end(), genContacts.begin(),
			[&](Contact& c) {return GenContact(c, state.position.block<3,1>(0,0));});

		//first handle the contact impulses (since their force is infinite)
		for (const auto& c : genContacts)
		{
			float badMomentum = -state.momentum.dot(c);
			if (badMomentum <= 0.f)
				continue;
			//should this be in the integrator?
			GenCoord dM = c*badMomentum;
			state.momentum += dM*(1.f + .8f); //restitution
			state.position += state.inverseIntertia * dM * simDt;
		}

		Eigen::VectorXf gravity = Eigen::VectorXf::Zero(m);
		gravity[2] = -9.8f * state.mass;

		Eigen::VectorXf b = -gravity;
		Eigen::VectorXf b2{ m };
		Eigen::VectorXf x = Eigen::VectorXf::Zero(m);
		std::vector<float> taus;
		std::vector<Eigen::VectorXf> QR;
		std::vector<Eigen::VectorXf> active;

		Eigen::VectorXf workspace{ m }, workspace2{ m };

		//index of current force
		Index k = 0;

		//First pass: forces that oppose gravity
		for (auto& c : genContacts)
		{
			if (c.dot(gravity) < -.01f)
			{
				workspace = c;
				for (Index j = 0; j < k; ++j)
				{
					workspace.tail(m - j).applyHouseholderOnTheLeft(
						QR[j].tail(m - j - 1), taus[j], workspace2.data());

					//is c linearly dependent with QR(0..j)?
					//alternatively, if the first element == the norm, since H preserves norm
					if (workspace.tail(m - j - 1).isZero())
						goto next;
				}

				//will the solution still be positive?
				float tau;
				workspace.tail(m - k).makeHouseholderInPlace(tau, workspace[k]);
				b2 = b;
				b2.tail(m - k).applyHouseholderOnTheLeft(
					workspace.tail(m - k - 1), tau, workspace2.data());
				Eigen::VectorXf xh = x.head(k + 1);
				xh = b2.head(k + 1);
				xh[k] /= workspace[k];
				xh.head(k) -= xh[k] * workspace.head(k);

				for (Index j = k - 1; j >= 0; --j)
				{
					xh[j] /= QR[j][j];
					xh.head(j) -= xh[j] * QR[j].head(j);
				}
				
				if (xh == xh.cwiseAbs())
				{
					b = b2; //swap?
					x.head(k + 1) = xh;
					QR.push_back(workspace);
					taus.push_back(tau);
					active.push_back(c);
					++k;
				}
			}
		next:
			;
		}

		GenCoord normalForce = GenCoord::Zero();
		for (Index j = 0; j < k; ++j)
        {
            GenCoord normj = active[j] * x[j];
			normalForce += normj;
            
            Vector3f lin = normj.block<3, 1>(0, 0);
            Vector3f rot = lin.cross(normj.block<3, 1>(3, 0)).normalized() * 2.f;
            debug.PushVector(state.position.block<3,1>(0,0), rot, {1, 1, 1});
            debug.PushVector(state.position.block<3,1>(0,0) + rot,
                lin, {1, 0, 0});
        }

		//if (k > 0)
		//	std::cerr << "\n\n" << k << "\n\n" << x << "\n\n" << normalForce << "\n\n";
		float T = (.5f * state.inverseIntertia * state.momentum).dot(state.momentum);
		float K = state.mass * 9.8f * state.position[2];
		std::cerr << "H = " << T + K << "\n";

		integrate(state, simTime,
			[=](const State& state, Time::clock::duration t)
		{
			return std::make_tuple(gravity, normalForce);
		});

		Transform xfrm;
		xfrm.pos = state.position.block<3,1>(0,0);
		xfrm.rot = RotToQuat(state.position.block<3, 1>(3, 0));
		xfrm.scale = 1; //FIXME
		position[obj.first].set(xfrm);
	}
    debug.End();
}

RigidBody::RigidBody(Position& position, NarrowPhase& narrowPhase,
    RenderPasses& passes)
	: paused(false), position(position), narrowPhase(narrowPhase), debug(passes)
{}

void RigidBody::Load(const Persist& persist)
{
	for (const auto& dat : persist.GetAll<RigidBody>())
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
	init.momentum = GenCoord::Zero();
	init.inverseIntertia.diagonal() <<
		1.f/mass, 1.f / mass, 1.f / mass,
		1.f/inertia, 1.f / inertia, 1.f / inertia;
	init.mass = mass; init.inertia = inertia;

	Transform xfrm = position[o].get();
	init.position << xfrm.pos, QuatToRot(xfrm.rot);

	data.try_emplace(o, init);
}

MAP_COMPONENT_BOILERPLATE(RigidBody, data);

template<>
const char* PersistSchema<RigidBody>::name = "rigidbody";
template<>
Columns PersistSchema<RigidBody>::cols = { "object", "mass", "inertia" };
