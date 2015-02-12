#include "stdafx.h"
#include "Mobile.hpp"

#include "Eigen/Core"

Mobile::MoveProxy Mobile::Create(const Transform& loc, std::vector<Render::LocationProxy> targets)
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
            *target = interp(dat.before, dat.loc, alpha);
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
	return mobile.data.get(ref)->loc;
}

const Transform& Mobile::MoveProxy::operator*() const
{
    return mobile.data.get(ref)->loc;
}

void Mobile::MoveProxy::Add(Render::LocationProxy target)
{
	auto& targets = mobile.data.get(ref)->targets;
	if (std::find(targets.begin(), targets.end(), target) == targets.end())
		mobile.data.get(ref)->targets.push_back(target);
}

void Mobile::MoveProxy::Remove(Render::LocationProxy target)
{
	auto& targets = mobile.data.get(ref)->targets;
	auto it = std::find(targets.begin(), targets.end(), target);
	if (it == targets.end())
		mobile.data.get(ref)->targets.erase(it);
}

Mobile::MoveProxy::MoveProxy(PermaRef ref, Mobile& mobile)
	: ref(ref), mobile(mobile)
{}
