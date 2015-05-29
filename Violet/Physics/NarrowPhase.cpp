#include "stdafx.h"
#include "NarrowPhase.hpp"
#include "File/Persist.hpp"
#include "Position.hpp"
#include "Geometry/Collide.hpp"

#include "Rendering/RenderPasses.hpp"

#include <iostream>

Matrix4f BoxMat(const AlignedBox3f& box)
{
	return Eigen::Affine3f{ Eigen::Translation3f{ box.min() }
	* Eigen::AlignedScaling3f{ box.max() - box.min() }
	}.matrix();
}

bool ConservativeOBBvsOBB1(const Matrix4f& l, const Matrix4f& r)
{
	using Eigen::Array3f;
	Matrix4f rToL = AffineInverse(l) * r;
	Array3f pt = rToL.block<3, 1>(0, 3);
	Eigen::Array33f frame = rToL.block<3, 3>(0, 0);
	
	return (frame.cwiseMax(0).rowwise().sum() + pt > Array3f::Zero()).all() //all max > 0
		&& (frame.cwiseMin(0).rowwise().sum() + pt < Array3f::Ones()).all(); //all min < 1
}

bool ConservativeOBBvsOBB(const Matrix4f& l, const Matrix4f& r)
{
	return ConservativeOBBvsOBB1(l, r) && ConservativeOBBvsOBB1(r, l);
}

std::vector<Vector3f> NarrowPhase::Query(Object a, Object b)
{
	Matrix4f apos = position[a].get().ToMatrix();
	Matrix4f bpos = position[b].get().ToMatrix();

	AlignedBox3f abb = *data.at(a).Tree().begin(),
		bbb = *data.at(b).Tree().begin();
	
	std::vector<DebugInst> insts = {
		{ apos * BoxMat(abb), {} },
		{ bpos * BoxMat(bbb), {} }
	};

	if (ConservativeOBBvsOBB(apos * BoxMat(abb), bpos * BoxMat(bbb)))
		insts[0].color = insts[1].color = Vector3f{ 1, 0, 0 };
	else
		insts[0].color = insts[1].color = Vector3f{ 0, 1, 0 };

	instances.Data(insts);

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
	for (auto& dat : persist.GetAll<NarrowPhase>())
		data.try_emplace(std::get<0>(dat), std::get<1>(dat));
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
	if (Has(obj))
		persist.Set<NarrowPhase>(obj, data.at(obj));
	else
		persist.Delete<NarrowPhase>(obj);
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
