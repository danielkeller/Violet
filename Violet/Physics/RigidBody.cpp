#include "stdafx.h"
#include "RigidBody.hpp"

#include "Position.hpp"
#include "Collision.hpp"
#include "File/Persist.hpp"

#include "Eigen/SparseQR"

#include <iostream>

using Eigen::VectorXf;
using Eigen::MatrixXf;

static const float simDt = std::chrono::duration<float>{ Time::dt }.count();

//Generalized contact for one active body
RigidBody::SparseV RigidBody::GenContact1(Object obj, Vector3f point, Vector3f normal) const
{
    int idx = data.at(obj).index;
    GenCoord genNorm;
    genNorm << normal, normal.cross(state.location.block<3, 1>(idx * 3, 0) - point);
    genNorm.normalize();
    
    SparseV ret{ static_cast<int>(state.momentum.rows()) };
    ret.reserve(6);
    for (int i = 0; i < 6; ++i)
        ret.insert(idx * 6 + i) = genNorm[i];
    return ret;
}

//The generalized contact normal is a translation and rotation that moves
//the contact point perpendicularly away from the surface. In the case of
//a collision between two free objects, it's the translation and rotation
//of both that moves the objects away from each other.
//Equivalently, if f() = 0 defines an implicit surface where the objects
//are touching, this is the gradient of f.
RigidBody::SparseV RigidBody::GenContact(const Contact& c) const
{
    if (!Has(c.b))
    {
        if (Has(c.a))
            return GenContact1(c.a, c.point, c.bNormal);
        else return{};
    }
    if (!Has(c.a))
        return GenContact1(c.b, c.point, c.aNormal);
    //else:
    
    int idxA = data.at(c.a).index;
    int idxB = data.at(c.b).index;
    
    //the derivative along n of the rotation of a vector v w.r.t the rotataton
    //angle theta, evaluated at theta=0, is ||v|| sin(alpha), where alpha is the
    //angle between v and n. Conveniently enough, the direction of the surface
    //normal in rotation coordinates is in cross(v, n), and n is already normalized.
    GenCoord a, b;
    a << c.bNormal, c.bNormal.cross(state.location.block<3, 1>(idxA*3, 0) - c.point);
    b << c.aNormal, c.aNormal.cross(state.location.block<3, 1>(idxB*3, 0) - c.point);
    
    SparseV ret{(int)data.size() * 6};
    ret.reserve(12);
    
    if (idxA < idxB)
    {
        for (int i = 0; i < 6; ++i)
            ret.insert(idxA*6 + i) = a[i];
        for (int i = 0; i < 6; ++i)
            ret.insert(idxB*6 + i) = b[i];
    }
    else
    {
        for (int i = 0; i < 6; ++i)
            ret.insert(idxA*6 + i) = a[i];
        for (int i = 0; i < 6; ++i)
            ret.insert(idxB*6 + i) = b[i];
    }
    
    ret /= ret.norm();
    return ret;
}

struct RigidBody::Derivative
{
	VectorXf force, drag;
	VectorXf velocity;
    VectorXf spin; //derivative of quaternion
};

void RigidBody::advance(State& s, const Derivative& d, float dt)
{
    s.momentum += d.force * dt;
    s.momentum += d.drag * dt;
	s.location += d.velocity*dt;
    s.orientation += d.spin*dt;
    
    //s.badWork += d.drag.dot(d.velocity)*dt;
}

//RK4 step
template<typename F>
RigidBody::Derivative RigidBody::step(const State& initial, float alpha, const Derivative* d,
	Time::clock::duration t, F forces)
{
	State s = initial;
    if (d) advance(s, *d, simDt*alpha);

	Time::clock::duration newT = t +
		std::chrono::duration_cast<Time::clock::duration>(Time::dt*alpha);

	Derivative ret;
    std::tie(ret.force, ret.drag) = forces(s, newT);
    
    //divide up the velocity
    VectorXf genVel = inverseInertia * s.momentum;
    ret.spin = VectorXf{ s.orientation.rows() };
    ret.velocity = VectorXf{ s.location.rows() };
    
    for (int i = 0, j = 0, k = 0; i < genVel.rows(); i += 6, j += 4, k += 3)
    {
        //the angular velocity must be transformed into the tangent space of the orientation
        ret.spin.block<4, 1>(j, 0) = (Quaternionf{0, genVel[i+3], genVel[i+4], genVel[i+5]}
                                      * Quaternionf{&s.orientation[j]}).coeffs() * .5f;
        
        ret.velocity.block<3, 1>(k, 0) = genVel.block<3, 1>(i, 0);
    }
	
	return ret;
}

//RK4 integrate
template<typename F>
void RigidBody::integrate(State& state, Time::clock::duration t, F forces)
{
    Derivative a, b, c, d;
	a = step(state, 0.f, nullptr, t, forces);
	b = step(state, 5.f, &a, t, forces);
	c = step(state, 5.f, &b, t, forces);
	d = step(state, 1.f, &c, t, forces);

	Derivative final;
#define RKINTERP(val) final.val = 1.f / 6.f * (a.val + 2.f*(b.val + c.val) + d.val)
	RKINTERP(force);
	RKINTERP(drag);
    RKINTERP(velocity);
    RKINTERP(spin);
    advance(state, final, simDt);
    
    //renormalize quaternions
    for (int i = 0; i < state.orientation.rows(); i += 4)
        state.orientation.block<4, 1>(i, 0).normalize();
}

void RigidBody::PhysTick(Time::clock::duration simTime)
{
    if (paused)
        return;
    
    debug.Begin();
    
    auto& contacts = collision.Contacts();

    //FIXME: This should be done by the editor
    for (auto& pair : data)
    {
        Transform xfrm = position[pair.first].get();
        auto pos = state.location.block<3, 1>(pair.second.index * 3, 0);
        if (pos != xfrm.pos) //someone is moving it
            state.momentum.block<6, 1>(pair.second.index * 6, 0).setZero(); //don't keep falling
        pos = xfrm.pos;
        state.orientation.block<4, 1>(pair.second.index * 4, 0) = xfrm.rot.coeffs();
    }
     
    //now handle free work penalty impulses
    /*for (auto& pair : data)
    {
        size_t idx = pair.second.index;
        if (badWork[idx] > 0)
        {
            //std::cerr << state.badWork << '\n';
            auto m = momentum.block<6, 1>(idx*6, 0);
            if (!m.isZero(.001f))
                m = m.normalized() * std::sqrt(std::max(
                m.squaredNorm()
                - 2.f * pair.second.mass * badWork[idx], 0.f));
            else
                m = GenCoord::Zero();
            
            //try taking it out of potential energy too
            badWork[idx] = 0;
        }
    }*/
    
    using Index = Eigen::MatrixXf::Index;
    //number of rows, degrees of freeedom
    ptrdiff_t m = state.momentum.rows();

    VectorXf b = -gravity;
    VectorXf b2{ m };
    Eigen::VectorXf x = Eigen::VectorXf::Zero(m);
    std::vector<float> taus;
    std::vector<Eigen::VectorXf> QR;
    std::vector<SparseV> active;

    VectorXf workspace{ m };
    VectorXf workspace2{ m };

    //index of current force
    Index k = 0;

    //First pass: forces that oppose gravity
    for (auto& contact : contacts)
    {
        SparseV c = GenContact(contact);
        
        //handle contact impulses
        float badMomentum = -c.dot(state.momentum);
        if (badMomentum > 0.f)
        {
            //should this be in the integrator?
            SparseV dM = c*badMomentum;
            state.momentum += dM*(1.f + .8f); //restitution
        }
        
        if (c.dot(gravity) < -.01f)
        {
            workspace = c;
            for (Index j = 0; j < k; ++j)
            {
                workspace.tail(m - j).applyHouseholderOnTheLeft(
                    QR[j].tail(m - j - 1), taus[j], workspace2.data());

                //is c linearly dependent with QR(0..j)?
                if (workspace.tail(m - j - 1).isZero())
                    goto next; //early out this loop and continue the outer loop
            }

            //form the householder reflector for the current contact
            float tau;
            workspace.tail(m - k).makeHouseholderInPlace(tau, workspace[k]);
            
            //reflect b by the current column (it is already reflected by
            //the previous columns)
            b2 = b;
            b2.tail(m - k).applyHouseholderOnTheLeft(
                workspace.tail(m - k - 1), tau, workspace2.data());
            Eigen::VectorXf xh = x.head(k + 1);
            xh = b2.head(k + 1);
            
            //doing this rowwise instead of columnwise might be faster if we early out
            
            //back substitute the current column
            xh[k] /= workspace[k];
            xh.head(k) -= xh[k] * workspace.head(k);
            
            //did we introduce a non-positive coefficient?
            if (xh[k] <= 0) goto next;

            //back substitute the previous columns
            for (Index j = k - 1; j >= 0; --j)
            {
                xh[j] /= QR[j][j];
                xh.head(j) -= xh[j] * QR[j].head(j);
                
                //did we introduce a non-positive coefficient?
                if (xh[j] <= 0) goto next;
            }

            //Success!
            b = b2; //swap?
            x.head(k + 1) = xh;
            QR.push_back(workspace);
            taus.push_back(tau);
            active.push_back(c);
            ++k;
        }
    next:
        ;
    }
    
    //Second pass will probably be very neccesary when there are more objects
    
    //k is now the number of active forces

    VectorXf normalForce = VectorXf::Zero(m);
    
    for (Index j = 0; j < k; ++j)
        normalForce += active[j] * x[j];

    //if (k > 0)
    //	std::cerr << "\n\n" << k << "\n\n" << x << "\n\n" << normalForce << "\n\n";
    //float T = (.5f * state.inverseIntertia * state.momentum).dot(state.momentum);
    //float K = state.mass * 9.8f * state.position[2];
    //std::cerr << "H = " << T + K << "\n";

    integrate(state, simTime,
        [=](const State& state, Time::clock::duration t)
    {
        return std::make_tuple(gravity, normalForce);
    });

    for (auto& pair : data)
    {
        Transform xfrm = position[pair.first].get();
        xfrm.pos = state.location.block<3, 1>(pair.second.index * 3, 0);
        xfrm.rot = &state.orientation[pair.second.index * 4];
        position[pair.first].set(xfrm);
    }

    debug.End();
}

RigidBody::RigidBody(Position& position, Collision& collision,
    RenderPasses& passes)
	: paused(false), position(position), collision(collision), debug(passes)
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
    int index;
    if (freeIndexes.size())
    {
        index = freeIndexes.back();
        freeIndexes.pop_back();
    }
    else
    {
        index = (int)data.size();
        
        VectorXf g{(index + 1) * 6};
        g << gravity, 0,0,0,0,0,0;
        gravity = std::move(g);
        
        VectorXf momentum{(index + 1) * 6};
        momentum << state.momentum, 0,0,0,0,0,0;
        state.momentum = std::move(momentum);
        
        VectorXf location{(index + 1) * 3};
        location << state.location, 0,0,0;
        state.location = std::move(location);
        
        VectorXf orientation{(index + 1) * 4};
        orientation << state.orientation, 0,0,0,0;
        state.orientation = std::move(orientation);
        
        VectorXf invIn{(index + 1) * 6};
        invIn << inverseInertia.diagonal(), 0,0,0,0,0,0;
        inverseInertia.diagonal() = invIn;
    }
    
    gravity.block<6, 1>(index * 6, 0).setZero();
    gravity[index * 6 + 2] = -9.8 * mass;
    
    state.momentum.block<3, 1>(index * 6, 0).setZero();
	inverseInertia.diagonal().block<6, 1>(index*6, 0) <<
		1.f/mass, 1.f / mass, 1.f / mass,
		1.f/inertia, 1.f / inertia, 1.f / inertia;
    
	data.try_emplace(o, Props{index, mass, inertia});
    
    
    //set z components
    //Eigen::VectorXf gravity(momentum.rows());
    //for (size_t idx = 0; idx < order.size(); ++idx)
    //    gravity[idx*6 + 2] = -9.8f * data[order[idx]].mass;
}

void RigidBody::Unload(const Persist& persist)
{
    for (const auto& dat : persist.GetAll<RigidBody>())
        data.erase(std::get<0>(dat));
}
bool RigidBody::Has(Object obj) const
{
    return data.count(obj) > 0;
}
void RigidBody::Remove(Object obj)
{
    freeIndexes.push_back(data[obj].index);
    data.erase(obj);
}

template<>
const char* PersistSchema<RigidBody>::name = "rigidbody";
template<>
Columns PersistSchema<RigidBody>::cols = { "object", "mass", "inertia" };
