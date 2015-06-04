#include "stdafx.h"
#include "Position.hpp"
#include "Utils/Profiling.hpp"
#include "File/Persist.hpp"

Matrix4f Transform::ToMatrix() const
{
	//double check this.
	return (Eigen::Translation3f(pos) * rot * Eigen::Scaling(scale)).matrix();
}

bool Transform::operator==(const Transform& other) const
{
	return (std::tie(pos, scale) == std::tie(other.pos, other.scale))
		&& rot.isApprox(other.rot); //?
}

bool Transform::operator!=(const Transform& other) const
{
	return !operator==(other);
}

std::ostream & operator<<(std::ostream &os, const Transform& p)
{
	return os << p.pos.transpose() << ", " << p.rot.w() << ' ' << p.rot.vec().transpose()
		<< ", " << p.scale;
}

magic_ptr<Transform> Position::operator[](Object obj)
{
	return make_magic(acc, obj);
}

using namespace std::placeholders;

Position::Position()
	//std::bind doesn't work because reasons
	: acc([this](Object o){return Get(o); }, [this](Object o, const Transform& t) { Set(o, t); })
{
}


const Transform& Position::Get(Object obj)
{
	return data[obj].loc;
}

void Position::Set(Object obj, const Transform& t)
{
	auto& dat = data[obj];
	dat.loc = t;
	dat.target.set(t);
}

void Position::Watch(Object obj, magic_ptr<Transform> w)
{
	data[obj].target += w;
}

void Position::Load(const Persist& persist)
{
	for (const auto& row : persist.GetAll<Position>())
		Set(std::get<0>(row), std::get<1>(row));
}

void Position::Save(Object obj, Persist& persist) const
{
	if (Has(obj))
		persist.Set<Position>(obj, data.at(obj).loc);
	else
		persist.Delete<Position>(obj);
}

MAP_COMPONENT_BOILERPLATE(Position, data)

template<>
const char* PersistSchema<Position>::name = "position";
template<>
Columns PersistSchema<Position>::cols = {"object", "transform"};