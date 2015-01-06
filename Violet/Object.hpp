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
    explicit Object(std::uint32_t v) : id(v) {}
    std::uint32_t Id() const {return id;}

	BASIC_EQUALITY(Object, id)

	bool operator<(const Object& other) const
	{ return id < other.id;	}

    friend std::ostream & operator<<(std::ostream &os, const Object& p)
    {
        return os << '[' << p.id << ']';
    }

    static const Object invalid;
    static const Object none;

    HAS_HASH
};

MEMBER_HASH(Object, id)

template <typename T>
using PlainComponent = std::vector < std::pair<Object, T> >;

#endif
