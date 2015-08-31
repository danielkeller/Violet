#ifndef RIGID_BODY_HPP
#define RIGID_BODY_HPP

#include "Core/Component.hpp"
#include "Containers/l_unordered_map.hpp"
#include "Core/Time.hpp"

#include "Utils/DebugBoxes.hpp"

#include "Eigen/SparseCore"

class Position;
class Collision;

struct Contact;

//TODO: "physics mesh" component

//generalized coordinates, stores 3 linear components and 3 rotation components
//aka, a Screw
using GenCoord = Eigen::Matrix<float, 6, 1>;

class RigidBody : public Component
{
public:
	RigidBody(Position&, Collision&, RenderPasses&);
	void Add(Object o, float mass, float inertia);
	void PhysTick(Time::clock::duration simTime);
    
    bool& Debug() { return debug.enabled; }
    bool paused;
    
    struct State
    {
        Eigen::VectorXf
            location, // in 3-vectors
            orientation, // in quaternions
            momentum, // in 6-vectors (linear[3], angular[3])
            badWork; // in scalars
    };
    
private:
	Position& position;
	Collision& collision;
    
    State state;
    
    Eigen::DiagonalMatrix<float, Eigen::Dynamic> inverseInertia; // in 6-vectors (mmmIII)
    
    struct Props
    {
        int index; //of the vector elements
        float mass, inertia;
    };
	l_unordered_map<Object, Props> data;
    std::vector<int> freeIndexes;
    
    using SparseV = Eigen::SparseVector<float>;
    //using SparseM = Eigen::SparseMatrix<float>;
    
    Eigen::VectorXf gravity;
    
    SparseV GenContact1(Object obj, Vector3f point, Vector3f normal) const;
    SparseV GenContact(const Contact& c) const;
    
    //integrator
    struct Derivative;
    void advance(State& s, const Derivative& d, float dt);
    template<typename F>
    Derivative step(const State& initial, float alpha, const Derivative* d,
                    Time::clock::duration t, F forces);
    template<typename F>
    void integrate(State& state, Time::clock::duration t, F forces);

	void Load(const Persist&);
	void Unload(const Persist&);
	bool Has(Object) const;
	void Save(Object, Persist&) const;
    void Remove(Object);
    
    DebugBoxes debug;
};

MAKE_PERSIST_TRAITS(RigidBody, Object, float, float)

#endif
