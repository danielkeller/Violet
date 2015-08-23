#ifndef AABB_HPP
#define AABB_HPP

//incremental AABB tree

#include "Containers/variant.hpp"

template<class T>
class AABBTree
{
public:
    struct Node;
    using iterator = Node*;
    struct query_iterator;
    
    iterator insert(Box3 box, T val);
    void erase(iterator);
    
    //iterator must not outlive query
    query_iterator query(const Box3& query) const {return{ tree.get(), &query };}
    query_iterator query_end() const { return{}; }
    
private:
    using NodePtr = std::unique_ptr<Node>;
    
    void newNode(NodePtr node, NodePtr& root);
    
    NodePtr tree;
};

template<class T>
struct AABBTree<T>::Node
{
    Box3
    box;
    Node* parent;
    
    using Ptr = std::unique_ptr<Node>;
    using Children = std::pair<Ptr, Ptr>;
    variant<Children, T> data;
    
    Node(Box3 box, Node* parent, Ptr l, Ptr r)
        : box(box), parent(parent), data(Children{std::move(l), std::move(r)})
    {
        left()->parent = this;
        right()->parent = this;
    }
    Node(Box3 box, Node* parent, T leaf)
        : box(box), parent(parent), data(leaf)
    {}
    
    bool leaf() const {return data.template is<T>();}
    Ptr& left() {return data.template get<Children>().first;}
    const Ptr& left() const {return data.template get<Children>().first;}
    Ptr& right() {return data.template get<Children>().second;}
    const Ptr& right() const {return data.template get<Children>().second;}
};

template<class T>
struct AABBTree<T>::query_iterator : public std::iterator<std::forward_iterator_tag, T>
{
    using Node = AABBTree::Node;
    using value_type = T;
    
    const Node* current;
    const Box3* query;
    
    query_iterator() : current(nullptr) {}
    query_iterator(const Node* c, const Box3* q) : current(c), query(q) {search();}
    BASIC_EQUALITY(query_iterator, current);
    
    query_iterator& operator++();
    query_iterator operator++(int) {auto ret = this; ++*this; return ret;}
    const T& operator*() {return current->data.template get<T>(); }
    const T* operator->() {return &**this;}
    
private:
    void backup();
    void search();
};

template<class T>
auto AABBTree<T>::insert(Box3 box, T val) -> iterator
{
    NodePtr node = std::make_unique<Node>(box, nullptr, val);
    iterator ret = node.get();
    if (!tree)
        tree = std::move(node);
    else
        newNode(std::move(node), tree);
    return ret;
}

inline float Area(const Box3& box)
{
    Vector3 dims = box.size();
    return dims.x * (dims.y + dims.z) + dims.y * dims.z;
}

template<class T>
void AABBTree<T>::newNode(NodePtr node, NodePtr& root)
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

template<class T>
void AABBTree<T>::erase(iterator it)
{
    if (!it->parent) //only node
    {
        tree.reset();
        return;
    }
    /* Do this:
             |
             X  <-+
            / \   |
     it -> X   O -+
     */
    
    //who owns the parent?
    NodePtr& parentPtr = !it->parent->parent ? tree :
        it->parent == it->parent->parent->left().get() ? it->parent->parent->left()
        : it->parent->parent->right();
    
    //detach other child
    NodePtr otherPtr = std::move(it == it->parent->left().get() ? it->parent->right() : it->parent->left());
    it = otherPtr.get();
    it->parent = it->parent->parent;
    parentPtr = std::move(otherPtr); //replace parent
    
    //refit the boxes
    for(it = it->parent; it; it = it->parent)
        it->box = it->left()->box.merged(it->right()->box);
}

//preorder traversal of the portion of the tree that overlaps the query, stopping on leaf nodes
template<class T>
auto AABBTree<T>::query_iterator::operator++() -> query_iterator&
{
    backup();
    search();
    return *this;
}

template<class T>
void AABBTree<T>::query_iterator::backup()
{
    //ascend until there's a sibling we can switch to
    while (true)
    {
        if (!current->parent)
        {
            //done
            current = nullptr;
            return;
        }
        else if (current == current->parent->left().get()
                 && query->intersects(current->parent->right()->box))
        {
            //switch to the right child
            current = current->parent->right().get();
            return;
        }
        else //keep backing up
            current = current->parent;
    }
}

template<class T>
void AABBTree<T>::query_iterator::search()
{
    //descend as far as possible, to the left first
    while (current && !current->leaf())
    {
        if (query->intersects(current->left()->box))
            current = current->left().get();
        else if (query->intersects(current->right()->box))
            current = current->right().get();
        else //stuck, try the next subtree
            backup();
    }
}

#endif
