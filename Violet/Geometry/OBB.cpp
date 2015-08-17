#include "stdafx.h"
#include "OBB.hpp"
#include "Core/Resource.hpp"

#include <iostream>

struct OBBTree::Resource : public ::Resource<OBBTree::Resource>
{
	TreeTy tree;
	Resource(std::string file);
	void LoadCache(std::string cacheFile);
	void SaveCache(std::string cacheFile);
    
    void RecTree(TreeTy::iterator it, Mesh::iterator begin, Mesh::iterator end);
};

OBBTree::Resource::Resource(std::string file)
	: ResourceTy(file)
{
	auto p = Profile("build OBB");

	Mesh m = LoadMesh(file);

	size_t height = static_cast<size_t>(
		std::max(1.f, std::ceilf(std::log2f(m.size()))));

    tree.reserve(height);
    
    RecTree(tree.begin(), m.begin(), m.end());
}

void OBBTree::Resource::RecTree(TreeTy::iterator it, Mesh::iterator begin, Mesh::iterator end)
{
    if (begin + 1 == end)
    {
        *it = *begin;
        return;
    }
    else
        *it = OBB{begin, end};
    
    //Get longest axis
    Vector3f::Index longestAxisIndex;
    it->get<OBB>().axes.colwise().norm().maxCoeff(&longestAxisIndex);
    
    Vector3f meshCentroid = centroid(begin, end);
    
    auto axInd = longestAxisIndex;
    auto middle = begin;
    
    while (true)
    {
        Vector3f ax = it->get<OBB>().axes.col(axInd).normalized();
        
        middle = std::partition(begin, end, [=](const Triangle& t)
        {
            return ::centroid(t).dot(ax) < meshCentroid.dot(ax);
        });
        
        axInd = (axInd + 1) % 3;
        
        //If one side has the whole mesh, split it differently
        //if (middle != begin && middle != end) //nondegenerate split
        if ((middle - begin) * 5 > end - begin && (end - middle) * 5 > end - begin) // better than 80-20
            break;
        if (axInd == longestAxisIndex) //no more axes to try
        {
            //arbitrary split that gives each half at least one triangle
            middle = begin + (end - begin) / 2;
            break;
        }
    }
    
    //If just one child is a triangle, it will be the left one
    if (middle + 1 == end)
    {
        RecTree(it.Left(), middle, end);
        RecTree(it.Right(), begin, middle);
    }
    else
    {
        RecTree(it.Left(), begin, middle);
        RecTree(it.Right(), middle, end);
    }
}

OBBTree::OBBTree(std::string file)
	: resource(Resource::FindOrMake(file))
{}

std::string OBBTree::Name() const
{
	return resource->Key();
}

const OBBTree::TreeTy& OBBTree::Tree() const
{
	return resource->tree;
}
