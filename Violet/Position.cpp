#include "stdafx.h"
#include "Position.hpp"
#include "Profiling.hpp"
#include "Persist.hpp"

magic_ptr<Transform> Position::operator[](Object obj)
{
	return make_magic(acc, obj);
}

using namespace std::placeholders;

Position::Position(Persist& persist)
	: persist(persist)
	//std::bind doesn't work because reasons
	, acc([this](Object o){return Get(o); }, [this](Object o, const Transform& t) { Set(o, t); })
{
	for (const auto& row : persist.GetAll<Position>())
		Set(std::get<0>(row), std::get<1>(row));
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

bool Position::Has(Object obj) const
{
	return data.count(obj) > 0;
}

void Position::Save(Object obj)
{
	if (Has(obj))
		persist.Set<Position>(obj, data[obj].loc);
	else
		persist.Delete<Position>(obj);
}

void Position::Remove(Object obj)
{
	data.erase(obj);
}

template<>
const char* PersistSchema<Position>::name = "position";
template<>
Columns PersistSchema<Position>::cols = {"object", "transform"};