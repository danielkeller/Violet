#ifndef OBJECT_H
#define OBJECT_H

#include <vector>
#include <ostream>

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

    friend std::ostream & operator<<(std::ostream &os, const Object& p)
    {
        return os << '[' << p.id << ']';
    }
};

template <typename T>
using PlainComponent = std::vector < std::pair<Object, T> >;

#endif
