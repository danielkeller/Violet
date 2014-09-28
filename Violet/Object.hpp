#include <vector>

class Object
{
	unsigned int id;
public:
	Object();

	bool operator==(const Object& other)
	{ return other.id == id; }

	bool operator!=(const Object& other)
	{ return !(*this == other); }
};

template <typename T>
using PlainComponent = std::vector < std::pair<Object, T> >;