#ifndef OBJECT_H
#define OBJECT_H

#include <cstdint>

class Persist;

class Object
{
	std::uint32_t id;
public:
	static void Init(Persist&);

    Object();
    explicit Object(std::uint32_t v) : id(v) {}
    std::uint32_t Id() const {return id;}

	BASIC_EQUALITY(Object, id)

	bool operator<(const Object& other) const
	{ return id < other.id;	}

	friend std::ostream & operator<<(std::ostream &os, const Object& p);
	friend std::string to_string(Object obj);

    static const Object invalid;
    static const Object none;
	
	static std::uint32_t next;

    HAS_HASH
};

MEMBER_HASH(Object, id)

MAKE_PERSIST_TRAITS(Object, Object)

class ObjectName
{
public:
	ObjectName(Persist&);

	std::string operator[](Object);
	Object operator[](const std::string&);
	void Rename(Object, const std::string&);
private:
	Persist& persist;
};

MAKE_PERSIST_TRAITS(ObjectName, Object, std::string)

#endif
