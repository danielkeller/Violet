#include "stdafx.h"
#include "NarrowPhase.hpp"
#include "File/Persist.hpp"
#include "Position.hpp"
#include "Geometry/Collide.hpp"

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

std::vector<Vector3f> NarrowPhase::Query(Object a, Object b)
{
	Matrix4f apos = position[a].get().ToMatrix();
	Matrix4f aInv = AffineInverse(position[a].get().ToMatrix());
	Matrix4f bpos = position[b].get().ToMatrix();
	Matrix4f bInv = AffineInverse(position[b].get().ToMatrix());

	std::vector<DebugInst> insts;
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
				insts.push_back({ bpos * OBBMat(*pair.second), Vector3f{ 1, .5f, 0 } });

				for (; ; pair.first.ToParent(), pair.second.ToParent())
				{
					Vector3f shiny{ dis(gen), dis(gen), dis(gen) };
					insts.push_back({ apos * OBBMat(*pair.first), shiny });
					insts.push_back({ bpos * OBBMat(*pair.second), shiny });

					if (pair.first.Top() || pair.second.Top())
						break;//goto done;
				}
			}
		}
		/*else
		{
			insts.push_back({ apos * OBBMat(*pair.first),  Vector3f{ 0, 1, 0 } });
			insts.push_back({ bpos * OBBMat(*pair.second), Vector3f{ 0, 1, 0 } });
		}*/
	}
	done:
	

	instances.Data(insts);
	dbgVao.NumInstances(static_cast<GLsizei>(insts.size()));

	//std::cerr << leavesToCheck.size() << '\n';

	return{};
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
	data.try_emplace(obj, mesh);
}

void NarrowPhase::Load(const Persist& persist)
{
	//for (auto& dat : persist.GetAll<NarrowPhase>())
	//	data.try_emplace(std::get<0>(dat), std::get<1>(dat));
}

void NarrowPhase::Unload(const Persist& persist)
{
	for (auto& dat : persist.GetAll<NarrowPhase>())
		data.erase(std::get<0>(dat));
}

bool NarrowPhase::Has(Object obj) const
{
	return data.count(obj) > 0;
}

void NarrowPhase::Save(Object obj, Persist& persist) const
{
	//if (Has(obj))
	//	persist.Set<NarrowPhase>(obj, data.at(obj));
	//else
	//	persist.Delete<NarrowPhase>(obj);
}

void NarrowPhase::Remove(Object obj)
{
	data.erase(obj);
}

template<>
const char* PersistSchema<NarrowPhase>::name = "narrowphase";
template<>
Columns PersistSchema<NarrowPhase>::cols = { "object", "aabb" };

template<>
const Schema AttribTraits<NarrowPhase::DebugInst>::schema = {
	AttribProperties{ "transform", GL_FLOAT, false, 0,                 { 4, 4 }, 4 * sizeof(float) },
	AttribProperties{ "color",     GL_FLOAT, false, 16 * sizeof(float),{ 3, 1 }, 0 },
};
