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

using Iter = NarrowPhase::TreeTy::TreeTy::const_iterator;
using IterPair = std::pair<Iter, Iter>;

struct TotalVolCmp
{
	bool operator()(const IterPair& l, const IterPair& r)
	{
		return l.first->squaredVolume() + l.second->squaredVolume()
			< r.first->squaredVolume() + r.second->squaredVolume();
	}
};

std::vector<Contact> NarrowPhase::Query(Object a, Object b) const
{
	if (!data.count(a) || !data.count(b))
		return{};

	Matrix4f apos = position[a].get().ToMatrix();
	Matrix4f aInv = AffineInverse(position[a].get().ToMatrix());
	Matrix4f bpos = position[b].get().ToMatrix();
	Matrix4f bInv = AffineInverse(position[b].get().ToMatrix());

	std::random_device rd;
	std::mt19937 gen(rd());
	gen.seed(0);
	std::uniform_real_distribution<float> dis(0, 1);
	
	auto it = data.at(a).Tree().begin();
	while (!it.Left().Left().Bottom()) it.ToLeft();
	
	std::vector<IterPair> leavesToCheck;

	std::priority_queue<IterPair, std::vector<IterPair>, TotalVolCmp> queue;
	queue.push({ data.at(a).Tree().begin(), data.at(b).Tree().begin() });

	while (!queue.empty())
	{
		auto pair = queue.top();
		queue.pop();
		
		if (ConservativeOBBvsOBB(
			apos * OBBMat(*pair.first), InvOBBMat(*pair.first) * aInv,
			bpos * OBBMat(*pair.second), InvOBBMat(*pair.second) * bInv))
		{
			if (!pair.first.Bottom() && 
				(pair.second.Bottom() || pair.first->volume() > pair.second->volume()))
			{
				queue.push({ pair.first.Left(), pair.second });
				queue.push({ pair.first.Right(), pair.second });
			}
			else if (!pair.second.Bottom())
			{
				queue.push({ pair.first, pair.second.Left() });
				queue.push({ pair.first, pair.second.Right() });
			}
			else
			{
				leavesToCheck.push_back(pair);
				
				insts.push_back({ apos * OBBMat(*pair.first),  Vector3f{ 1, .5f, 0 } });
				insts.push_back({ bpos * OBBMat(*pair.second), Vector3f{ 0, 1, 0 } });
			}
		}
	}
	
	std::vector<Contact> ret;

	for (const auto& leafPair : leavesToCheck)
	{
		for (const auto& aTri : leafPair.first.Leaf())
		{
			Triangle aWorld = TransformTri(aTri, apos);
			for (const auto& bTri : leafPair.second.Leaf())
			{
				Triangle bWorld = TransformTri(bTri, bpos);
				auto pair = ContactPoint(aWorld, bWorld);
				if (pair.second)
				{
					ret.push_back({ pair.first,
						TriNormal(aWorld).normalized(), TriNormal(bWorld).normalized() });

					Matrix4f normVis = Matrix4f::Zero();
					normVis.block<3, 1>(0, 3) = pair.first;
					normVis(3, 3) = 1;

					normVis.block<3, 1>(0, 0) = ret.back().aNormal;
					insts.push_back({ normVis, { 1, .5f, 1 } });
					normVis.block<3, 1>(0, 0) = ret.back().bNormal;
					insts.push_back({ normVis, { 0, 1, 1 } });
				}
			}
		}
	}

	return ret;
}

std::vector<Contact> NarrowPhase::QueryAll(Object a) const
{
	insts.clear();

	std::vector<Contact> ret;
	for (const auto& obj : data)
		if (obj.first != a)
		{
			auto contacts = Query(a, obj.first);
			ret.insert(ret.begin(), contacts.begin(), contacts.end());
		}

	instances.Data(insts);
	dbgVao.NumInstances(static_cast<GLsizei>(insts.size()));
	return ret;
}

NarrowPhase::NarrowPhase(Position& position, RenderPasses& passes)
	: position(position), dbgMat("NarrowPhaseDebug", "assets/color")
	, dbgVao(dbgMat.Shader(), WireCube), instances(2)
{
	dbgVao.BindInstanceData(dbgMat.Shader(), instances);

	passes.CreateCustom(debugObj, [&](float)
	{
		dbgMat.use();
		dbgVao.Draw();
	});
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

template<>
const Schema AttribTraits<NarrowPhase::DebugInst>::schema = {
	AttribProperties{ "transform", GL_FLOAT, false, 0,                 { 4, 4 }, 4 * sizeof(float) },
	AttribProperties{ "color",     GL_FLOAT, false, 16 * sizeof(float),{ 3, 1 }, 0 },
};
