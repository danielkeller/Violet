#include "stdafx.h"
#include "Mobile.hpp"

#include "Eigen/Core"

magic_ptr<Matrix4f>& Mobile::operator[](Object obj)
{
	auto pair = data.try_emplace(obj, ObjData{});
	auto it = pair.first;
	if (pair.second) //emplace happened
	{
		//if we remove an object, this will add it back in if the magic_ptr
		//is assigned. this may or may not be okay.
		static accessor<Transform, Object> access = {
			[this](Object o) { assert(false && "Should not be called");  return Transform{}; },
			[this](Object o, const Transform& val)
			{
				auto p = data.try_emplace(o);
				auto& dat = p.first->second;
				if (p.second) //just inserted
					dat.before = dat.loc = val; //prevent stuttering
				else //already there
					dat.loc = val;
			} };
		position[obj] += magic_ptr<Transform>{access, obj};
	}

	return it->second.target;
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
		dat.second.target.set(interp(dat.second.before, dat.second.loc, alpha));
	cameraMat = interp(cameraBefore, cameraLoc, alpha);
}

void Mobile::Tick()
{
	for (auto& dat : data)
		dat.second.before = dat.second.loc;
	cameraBefore = cameraLoc;
}
