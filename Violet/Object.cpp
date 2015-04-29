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
	if (persist.Exists<ObjectName>(obj))
		return std::get<1>(persist.Get<ObjectName>(obj));
	else
	{
		static const std::string initName = "object";
		persist.Set<ObjectName>(obj, initName);
		return initName;
	}
}

Object ObjectName::operator[](const std::string& str)
{
	auto resultSet = persist.GetSome<ObjectName>("name", str);
	auto result = resultSet.begin();
	if (result)
		return std::get<0>(*result);
	else
	{
		Object newObj;
		persist.Set<ObjectName>(newObj, str);
		return newObj;
	}
}

void ObjectName::Rename(Object obj, const std::string& str)
{
	persist.Set<ObjectName>(obj, str);
}

template<>
const char* PersistSchema<ObjectName>::name = "objectname";
template<>
Columns PersistSchema<ObjectName>::cols = {"object", "name"};