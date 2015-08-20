#ifndef BROAD_PHASE_HPP
#define BROAD_PHASE_HPP

#include "Object.hpp"
#include "Geometry/Shapes.hpp"

#include "Utils/DebugBoxes.hpp"

class RenderPasses;
class NarrowPhase;

class BroadPhase
{
public:
    BroadPhase(RenderPasses&, NarrowPhase&);
    ~BroadPhase();
    void Add(Object);
    
private:
    struct Node;
    using NodePtr = std::unique_ptr<Node>;
    
    void newNode(NodePtr node, NodePtr& root);
    void show(const NodePtr& node, bool alt);
    
    NarrowPhase& narrow;
    
    NodePtr tree;
    
    mutable DebugBoxes debug;
};

#endif
