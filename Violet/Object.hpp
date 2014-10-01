#ifndef OBJECT_H
#define OBJECT_H

#include <vector>

class Object
{
	unsigned int id;
public:
	Object();

	bool operator==(const Object& other) const
	{ return other.id == id; }

	bool operator!=(const Object& other) const
	{ return !(*this == other); }

	bool operator<(const Object& other) const
	{ return id < other.id;	}
};

template <typename T>
using PlainComponent = std::vector < std::pair<Object, T> >;

#endif