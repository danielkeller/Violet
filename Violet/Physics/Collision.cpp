#include "stdafx.h"
#include "Collision.hpp"
#include "File/Persist.hpp"
#include "Position.hpp"
#include "Geometry/Collide.hpp"

#include "Utils/Template.hpp"
#include "Rendering/RenderPasses.hpp"

#include <iostream>
#include <queue>
#include <random>

//fixme: 1-tri meshes

void Collision::NarrowPhase(Object a, Object b)
{
	Transform apos = position[a].get();
	Transform bpos = position[b].get();

    nodesToCheck.clear(); //avoid reallocation
	nodesToCheck.push_back({ data.at(a).Tree().begin(), data.at(b).Tree().begin() });
    
    //if (ConservativeOBBvsOBB(apos * data.at(a).Tree().begin()->get<OBB>(), bpos * data.at(b).Tree().begin()->get<OBB>()))
    {
        //debug.PushInst({ (apos * data.at(a).Tree().begin()->get<OBB>()).matrix(), Vector3f{ 1, 1, 1 } });
        //debug.PushInst({ (bpos * data.at(b).Tree().begin()->get<OBB>()).matrix(), Vector3f{ 1, 1, 1 } });
    }
    
	while (!nodesToCheck.empty())
	{
        Iter aIt, bIt;
        std::tie(aIt, bIt) = nodesToCheck.back();
		nodesToCheck.pop_back();
		
        if (aIt->is<Triangle>()) //leaf vs leaf
        {
            Triangle aWorld = TransformTri(aIt->get<Triangle>(), apos.ToMatrix());
            Triangle bWorld = TransformTri(bIt->get<Triangle>(), bpos.ToMatrix());
            auto pair = ContactPoint(aWorld, bWorld);
            if (pair.second)
            {
                //TODO: first-contact early out
                result.push_back({ a, b, pair.first,
                    TriNormal(aWorld).normalized(), TriNormal(bWorld).normalized() });
                
                //debug.PushVector(pair.first, result.back().aNormal, { 1, .5f, 1 });
                //debug.PushVector(pair.first, result.back().bNormal, { 0, 1, 1 });
            }
        }
		else if (ConservativeOBBvsOBB(apos * aIt->get<OBB>(), bpos * bIt->get<OBB>()))
		{
            //Cases: Box Box, Tri Box, Tri Tri
            
			if (aIt.Left()->is<OBB>() &&
				(bIt.Left()->is<Triangle>() || aIt->get<OBB>().volume() > bIt->get<OBB>().volume()))
			{
				nodesToCheck.push_back({ aIt.Left(), bIt });
				nodesToCheck.push_back({ aIt.Right(), bIt });
			}
			else if (bIt.Left()->is<OBB>())
			{
				nodesToCheck.push_back({ aIt, bIt.Left() });
				nodesToCheck.push_back({ aIt, bIt.Right() });
			}
			else //each has at least one triangle
            {
                nodesToCheck.push_back({aIt.Left(), bIt.Left()});
                
                if (aIt.Right()->is<Triangle>())
                    nodesToCheck.push_back({aIt.Right(), bIt.Left()});
                else
                    nodesToCheck.push_back({aIt.Right(), bIt});
                
                if (bIt.Right()->is<Triangle>())
                {
                    nodesToCheck.push_back({aIt.Left(), bIt.Right()});
                    if (aIt.Right()->is<Triangle>())
                        nodesToCheck.push_back({aIt.Right(), bIt.Right()});
                }
                else
                    nodesToCheck.push_back({aIt, bIt.Right()});
				/*
                Matrix aDbgM = (bpos * bIt->get<OBB>()).matrix();
                Matrix4f aDbg;
                aDbg << aDbgM(0, 0), aDbgM(0, 1), aDbgM(0, 2), aDbgM(0, 3),
                    aDbgM(1, 0), aDbgM(1, 1), aDbgM(1, 2), aDbgM(1, 3),
                    aDbgM(2, 0), aDbgM(2, 1), aDbgM(2, 2), aDbgM(2, 3),
                    0, 0, 0, 1;
                
                debug.PushInst({ aDbg, Vector3f{ 1, .5f, 0 } });*/
                //debug.PushInst({ (bpos * bIt->get<OBB>()).matrix(), Vector3f{ 0, 1, 0 } });
			}
		}
	}
}

Box3 Loosen(const Box3& box)
{
    Vector3 size = box.size() * .1f; //magic fudge factor
    return{ box.min - size, box.max + size };
}

void Collision::PhysTick()
{
    debug.Begin();
    result.clear();
    
    //rejigger leaves that don't fit their object
    for (auto& pair : broadLeaves)
    {
        if (!pair.second->box.contains(Bound(pair.first)))
        {
            broadTree.erase(pair.second);
            pair.second = broadTree.insert(Loosen(Bound(pair.first)), pair.first);
        }
    }
    
    //don't do the same collision test two ways
    std::unordered_set<std::pair<Object, Object>> toTest;
    
    for (auto& pair : data)
    {
        Box3 query = Bound(pair.first);
        for (auto it = broadTree.query(query); it != broadTree.query_end(); ++it)
            if (*it != pair.first)
                toTest.insert({std::min(*it, pair.first), std::max(*it, pair.first)});
    }
    
    for (auto& pair : toTest)
        NarrowPhase(pair.first, pair.second);
    
    debug.End();
}

Box3 Collision::Bound(Object obj) const
{
    return (*position[obj] * data.at(obj).Tree().begin()->get<OBB>()).Bound();
}

Collision::Collision(Position& position, RenderPasses& passes)
	: position(position), debug(passes)
{
    //debug.enabled = true;
}

void Collision::Add(Object obj, OBBTree mesh)
{
    if (!data.count(obj))
    {
        data.emplace(obj, std::move(mesh));
        auto it = broadTree.insert(Bound(obj), obj);
        broadLeaves.try_emplace(obj, it);
    }
}

void Collision::Load(const Persist& persist)
{
	for (const auto& dat : persist.GetAll<Collision>())
        Add(std::get<0>(dat), std::move(std::get<1>(dat)));
}

void Collision::Save(Object obj, Persist& persist) const
{
	if (Has(obj))
		persist.Set<Collision>(obj, data.at(obj));
	else
		persist.Delete<Collision>(obj);
}

MAP_COMPONENT_BOILERPLATE(Collision, data)

template<>
const char* PersistSchema<Collision>::name = "collision";
template<>
Columns PersistSchema<Collision>::cols = { "object", "mesh" };

