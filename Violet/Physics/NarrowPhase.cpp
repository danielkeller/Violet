#include "stdafx.h"
#include "NarrowPhase.hpp"
#include "File/Persist.hpp"
#include "Position.hpp"
#include "Geometry/Collide.hpp"

#include "Utils/Math.hpp"
#include "Rendering/RenderPasses.hpp"

#include <iostream>
#include <queue>
#include <random>

//fixme: 1-tri meshes

std::vector<Contact> NarrowPhase::Query(Object a, Object b) const
{
	if (!data.count(a) || !data.count(b))
		return{};

	Transform apos = position[a].get();
	Transform bpos = position[b].get();

    nodesToCheck.clear(); //avoid reallocation
	nodesToCheck.push_back({ data.at(a).Tree().begin(), data.at(b).Tree().begin() });
    
    std::vector<Contact> ret;
    
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
                ret.push_back({ pair.first,
                    TriNormal(aWorld).normalized(), TriNormal(bWorld).normalized() });
                
                debug.PushVector(pair.first, ret.back().aNormal, { 1, .5f, 1 });
                debug.PushVector(pair.first, ret.back().bNormal, { 0, 1, 1 });
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
				
                debug.PushInst({ (apos * aIt->get<OBB>()).matrix(), Vector3f{ 1, .5f, 0 } });
                debug.PushInst({ (bpos * bIt->get<OBB>()).matrix(), Vector3f{ 0, 1, 0 } });
			}
		}
	}

	return ret;
}

std::vector<Contact> NarrowPhase::QueryAll(Object a) const
{
    debug.Begin();

	std::vector<Contact> ret;
	for (const auto& obj : data)
		if (obj.first != a)
		{
			auto contacts = Query(a, obj.first);
			ret.insert(ret.begin(), contacts.begin(), contacts.end());
		}

    debug.End();
    
	return ret;
}

NarrowPhase::NarrowPhase(Position& position, RenderPasses& passes)
	: position(position), debug(passes)
{}

AlignedBox3f NarrowPhase::Bound(Object obj)
{
    return (*position[obj] * data.at(obj).Tree().begin()->get<OBB>()).Bound();
}

void NarrowPhase::Add(Object obj, std::string mesh)
{
    if (!data.count(obj))
        data.emplace(obj, mesh);
}

void NarrowPhase::Load(const Persist& persist)
{
	for (const auto& dat : persist.GetAll<NarrowPhase>())
        if (!data.count(std::get<0>(dat)))
            data.emplace(std::get<0>(dat), std::get<1>(dat));
}

void NarrowPhase::Save(Object obj, Persist& persist) const
{
	if (Has(obj))
		persist.Set<NarrowPhase>(obj, data.at(obj));
	else
		persist.Delete<NarrowPhase>(obj);
}

MAP_COMPONENT_BOILERPLATE(NarrowPhase, data)

template<>
const char* PersistSchema<NarrowPhase>::name = "narrowphase";
template<>
Columns PersistSchema<NarrowPhase>::cols = { "object", "obb" };

