#ifndef COLLIDE_HPP
#define COLLIDE_HPP
#include <tuple>
#include "Shapes.hpp"

std::array<Vector3f, 0> Axes(Vector3f);
LineSegment Project(Vector3f v, Vector3f ax);
std::array<Vector3f, 4> Axes(const Triangle& t);
LineSegment Project(const Triangle& t, Vector3f ax);
std::array<Vector3f, 3> Axes(const AlignedBox3f&);
LineSegment Project(const AlignedBox3f& box, Vector3f ax);

//The separating axis theorem
template<class Shape1, class Shape2>
bool Intersects(const Shape1& a, const Shape2& b)
{
	for (const Vector3f& ax : Axes(a))
		if (!Intersects(Project(a, ax), Project(b, ax)))
			return false;
	for (const Vector3f& ax : Axes(b))
		if (!Intersects(Project(a, ax), Project(b, ax)))
			return false;
	return true;
}

//line segment intersection
template<>
inline bool Intersects(const LineSegment& a, const LineSegment& b)
{
	return b.first <= a.second && b.second >= a.first;
}

template<>
inline bool Intersects(const AlignedBox3f& a, const AlignedBox3f& b)
{
	return a.intersection(b).volume() > 0.f;
}

bool ApproxIntersects(const AlignedBox3f& a, const Triangle& b);

#endif