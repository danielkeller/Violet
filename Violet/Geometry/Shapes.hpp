#ifndef SHAPES_HPP
#define SHAPES_HPP

#include <array>

using LineSegment = std::pair<float, float>;

struct Triangle
{
	Vector3f q, r, s;
};

struct Box
{
	Vector3f a, b;
};

//setwise and (intersection)
inline Box operator&(const Box& l, const Box& r)
{
	return Box{l.a.cwiseMax(r.a), l.b.cwiseMin(r.b)};
}

//setwise or (union)
inline Box operator|(const Box& l, const Box& r)
{
	return Box{ l.a.cwiseMin(r.a), l.b.cwiseMax(r.b) };
}

#endif
