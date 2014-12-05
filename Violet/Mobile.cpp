#include "stdafx.h"
#include "Mobile.hpp"

#include "Eigen/Core"

Mobile::MoveProxy Mobile::Add(const Transform& loc, std::vector<Render::LocationProxy> targets)
{
	auto ref = data.emplace_back(ObjData{ targets, loc, loc });
	return MoveProxy{ ref, *this };
}

Matrix4f Mobile::interp(const Transform& before, const Transform& loc, float alpha)
{
	return (
		Eigen::Translation3f((1 - alpha)*before.pos + alpha*loc.pos)
		* before.rot.slerp(alpha, loc.rot)
		* Eigen::Scaling((1 - alpha)*before.scale + alpha*loc.scale)
		).matrix();
}

void Mobile::Update(float alpha)
{
	for (auto& dat : data)
        for (auto& target : dat.targets)
            target = interp(dat.before, dat.loc, alpha);
	cameraMat = interp(cameraBefore, cameraLoc, alpha);
}

void Mobile::Tick()
{
	for (auto& dat : data)
		dat.before = dat.loc;
	cameraBefore = cameraLoc;
}

Transform& Mobile::MoveProxy::operator*()
{
	return ref.get(mobile.data)->loc;
}

Mobile::MoveProxy::MoveProxy(PermaRef ref, Mobile& mobile)
	: ref(ref), mobile(mobile)
{}
