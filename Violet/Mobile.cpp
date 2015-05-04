#include "stdafx.h"
#include "Mobile.hpp"

#include "Eigen/Core"

Matrix4f Mobile::interp(const Transform& before, const Transform& loc, float alpha)
{
	return (
		Eigen::Translation3f((1 - alpha)*before.pos + alpha*loc.pos)
		* before.rot.slerp(alpha, loc.rot)
		* Eigen::Scaling((1 - alpha)*before.scale + alpha*loc.scale)
		).matrix();
}