#include "stdafx.h"
#include "Object.hpp"

#include "Persist.hpp"

//Warning: if Object does an insert, everything else will get deleted

Object::Object()
{
	if (next == invalid.Id())
		throw std::logic_error("Object::Init not called");

	id = next++;
}

void Object::Init(Persist& persist)
{
	next = persist.NextObject().Id();
}

const Object Object::invalid{static_cast<std::uint32_t>(-1)};
const Object Object::none{static_cast<std::uint32_t>(-2)};

std::uint32_t Object::next = static_cast<std::uint32_t>(-1);

template<>
const char* PersistSchema<Object>::name = "object";
template<>
Columns PersistSchema<Object>::cols = { "object" };


ObjectName::ObjectName(Persist& persist)
	: persist(persist)
{}

std::string ObjectName::operator[](Object obj)
{
	return std::get<0>(*persist.GetSome<ObjectName>("object", obj).begin());
}

Object ObjectName::operator[](const std::string& str)
{
	if (persist.Exists<ObjectName>(str))
		return std::get<1>(persist.Get<ObjectName>(str));
	else
	{
		Object newObj;
		persist.Set<ObjectName>(str, newObj);
		return newObj;
	}
}

void ObjectName::Rename(Object obj, const std::string& str)
{
	persist.Set<ObjectName>(str, obj);
}

template<>
const char* PersistSchema<ObjectName>::name = "objectname";
template<>
Columns PersistSchema<ObjectName>::cols = {"name", "object"};