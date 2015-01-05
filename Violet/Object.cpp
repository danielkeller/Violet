#include "stdafx.h"
#include "Object.hpp"

Object::Object()
{
	static unsigned int next = 0;
	id = next++;
}

const Object Object::invalid{static_cast<std::uint32_t>(-1)};
const Object Object::none{static_cast<std::uint32_t>(-2)};
