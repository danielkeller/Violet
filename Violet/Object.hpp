#ifndef OBJECT_H
#define OBJECT_H

#include <vector>
#include <ostream>
#include <cstdint>

class Object
{
	std::uint32_t id;
public:
	Object();

	BASIC_EQUALITY(Object, id)

	bool operator<(const Object& other) const
	{ return id < other.id;	}

    friend std::ostream & operator<<(std::ostream &os, const Object& p)
    {
        return os << '[' << p.id << ']';
    }

    template<class T>
    friend struct std::hash;
};

template<>
struct std::hash<Object>
{
    size_t operator()(Object& o)
        { return std::hash<std::uint32_t>()(o.id);}
};

template <typename T>
using PlainComponent = std::vector < std::pair<Object, T> >;

#endif
