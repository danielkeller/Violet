#include "stdafx.h"
#include "Position.hpp"
#include "Profiling.hpp"
#include "Persist.hpp"

magic_ptr<Transform>& Position::operator[](Object obj)
{
	auto pair = data.emplace(std::piecewise_construct, std::tie(obj), std::tie());
	auto it = pair.first;
	if (pair.second) //emplace happened
	{
		//Other objects can now add to this magic_ptr to get setter notifications
		static accessor<Transform, Object> acc
		{
			[this](Object o) {return data[o].loc; },
			[this](Object o, const Transform& t) { data[o].loc = t; }
		};
		it->second.target = make_magic(acc, obj);
	}
	return it->second.target;
}

Position::Position(Persist& persist)
	: persist(persist)
{
	for (const auto& row : persist.GetAll<Position>())
		(*this)[std::get<0>(row)].set(std::get<1>(row));
}

void Position::Save(Object obj)
{
	persist.Set<Position>(obj, data[obj].loc);
}

template<>
const char* PersistSchema<Position>::name = "position";
template<>
Columns PersistSchema<Position>::cols = {"object", "transform"};