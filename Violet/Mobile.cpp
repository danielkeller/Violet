#include "stdafx.h"
#include "Mobile.hpp"

Mobile::MoveProxy Mobile::Add(const Transform& loc, Render::LocationProxy target)
{
	auto ref = data.emplace_back(ObjData{ target, loc, loc });
	return MoveProxy{ ref, *this };
}

void Mobile::Update(float alpha)
{
	for (auto& dat : data)
	{
		dat.target = (
			Eigen::Translation3f((1-alpha)*dat.before.pos + alpha*dat.loc.pos)
			* dat.before.rot.slerp(alpha, dat.loc.rot)
			* Eigen::Scaling((1 - alpha)*dat.before.scale + alpha*dat.loc.scale)
			).matrix();
	}
}

void Mobile::Tick()
{
	for (auto& dat : data)
		dat.before = dat.loc;
}

Transform& Mobile::MoveProxy::operator*()
{
	return ref.get(mobile.data)->loc;
}

Mobile::MoveProxy::MoveProxy(PermaRef ref, Mobile& mobile)
	: ref(ref), mobile(mobile)
{}