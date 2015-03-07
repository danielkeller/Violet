#include "stdafx.h"
#include "Mobile.hpp"

#include "Eigen/Core"

magic_ptr<Matrix4f>& Mobile::operator[](Object obj)
{
	auto pair = data.try_emplace(obj, ObjData{});
	auto it = pair.first;
	if (pair.second) //emplace happened
	{
		it->second.before = it->second.loc = *position[obj];
		//if we remove an object, this will add it back in if the magic_ptr
		//is assigned. this may or may not be okay.
		static accessor<Transform, Object> access = {
			[this](Object o) { return data[o].loc; },
			[this](Object o, const Transform& val) { data[o].loc = val; }
		};
		//we're perfectly capable to keeping track of the position ourself
		position[obj] = make_magic(access, obj);
		
		//need to do this to support pulling the location
		//however it keeps the combined accessor trick from working
		//so we just overwrite it in Render. It's kind of dumb but it works
		static accessor<Matrix4f, Object> targetaccess = {
			[this](Object o)
			{ 
				return interp(data[o].before, data[o].loc, alpha);
			},
			[this](Object o, const Matrix4f& val) {}
		};

		it->second.target = make_magic(targetaccess, obj);
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

void Mobile::Update(float a)
{
	alpha = a;
	for (auto& dat : data)
		dat.second.target.set(interp(dat.second.before, dat.second.loc, alpha));
}

void Mobile::Tick()
{
	for (auto& dat : data)
		dat.second.before = dat.second.loc;
}
