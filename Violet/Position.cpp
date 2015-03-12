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
	persist.Track<Position>();

	for (const auto& row : persist.GetAll<Position>())
		(*this)[std::get<0>(row)].set(std::get<1>(row));
}

void Position::Save(Object obj)
{
	persist.Set<Position>(obj, data[obj].loc);
}

const char* PersistSchema<Position>::name = "position";
const std::initializer_list<Column> PersistSchema<Position>::cols =
	{ objKey, { "transform", Column::BLOB } };