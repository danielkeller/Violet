#include "stdafx.h"
#include "BroadPhase.hpp"

#include "Containers/variant.hpp"

#include "Rendering/RenderPasses.hpp"

#include "NarrowPhase.hpp"

struct BroadPhase::Node
{
    AlignedBox3f box;
    Node* parent;
    
    using Ptr = std::unique_ptr<Node>;
    using Children = std::pair<Ptr, Ptr>;
    variant<Children, Object> data;
    
    Node(AlignedBox3f box, Node* parent, Ptr l, Ptr r)
        : box(box), parent(parent), data(Children{std::move(l), std::move(r)})
    {
        left()->parent = this;
        right()->parent = this;
    }
    Node(AlignedBox3f box, Node* parent, Object obj)
    : box(box), parent(parent), data(obj)
    {}
    
    bool leaf() {return data.is<Object>();}
    Ptr& left() {return data.get<Children>().first;}
    Ptr& right() {return data.get<Children>().second;}
};

BroadPhase::BroadPhase(RenderPasses& passes, NarrowPhase& narrow)
    : narrow(narrow), debug(passes)
{
    debug.enabled = true;
}

BroadPhase::~BroadPhase() = default;

void BroadPhase::Add(Object obj)
{
    NodePtr node = std::make_unique<Node>(narrow.Bound(obj), nullptr, obj);
    if (!tree)
        tree = std::move(node);
    else
        newNode(std::move(node), tree);
    
    debug.Begin();
    show(tree, true);
    debug.End();
}

void BroadPhase::show(const NodePtr& node, bool alt)
{
    debug.PushInst({BoxMat(node->box), Vector3f{1, 1, alt ? 1 : .7f}});
    if (!node->leaf())
    {
        show(node->left(), !alt);
        show(node->right(), !alt);
    }
}

//proportional surface area
float Area(const AlignedBox3f& box)
{
    Vector3f dims = box.sizes();
    return dims.x() * (dims.y() + dims.z()) + dims.y() * dims.z();
}

void BroadPhase::newNode(NodePtr node, NodePtr& root)
{
    if (!root->leaf())
    {
        //3 possible arrangements:
        float cur = Area(root->box) + Area(node->box);
        float left = Area(root->left()->box.merged(node->box)) + Area(root->right()->box);
        float right = Area(root->right()->box.merged(node->box)) + Area(root->left()->box);
        
        if (left < cur && left < right)
        {
            newNode(std::move(node), root->left());
            root->box = root->left()->box.merged(root->right()->box);
            return;
        }
        else if (right < cur && right < left)
        {
            newNode(std::move(node), root->right());
            root->box = root->left()->box.merged(root->right()->box);
            return;
        }
        //else fall through
    }
    
    NodePtr newRoot = std::make_unique<Node>(root->box.merged(node->box), root->parent, std::move(node), std::move(root));
    root = std::move(newRoot);
}
