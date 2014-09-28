#include "stdafx.h"
#include "Object.hpp"

Object::Object()
{
	static unsigned int next = 0;
	id = next++;
}