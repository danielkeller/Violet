#ifndef COLLIDE_HPP
#define COLLIDE_HPP
#include <tuple>
#include "Shapes.hpp"

//The separating axis theorem
template<class Shape1, class Shape2>
inline bool Intersects(const Shape1& a, const Shape2& b)
{
	for (const Vector3f& ax : Axes(a))
		if (!Intersects(Project(a, ax), Project(b, ax)))
			return false;
	for (const Vector3f& ax : Axes(b))
		if (!Intersects(Project(a, ax), Project(b, ax)))
			return false;
	return true;
}

std::array<Vector3f, 0> Axes(Vector3f)
{
	return{};
}

LineSegment Project(Vector3f v, Vector3f ax)
{
	return{ v.dot(ax), v.dot(ax) };
}

std::array<Vector3f, 4> Axes(const Triangle& t)
{
	auto face = (t.q - t.r).cross(t.s - t.r).normalized();
	return{
		face, //If I put braces around this the compiler crashes
		{ face.cross(t.q - t.r).normalized() },
		{ face.cross(t.r - t.s).normalized() },
		{ face.cross(t.s - t.q).normalized() }
	};
}

LineSegment Project(const Triangle& t, Vector3f ax)
{
	auto projs = { ax.dot(t.q), ax.dot(t.r), ax.dot(t.s) };
	return{ std::min(projs), std::max(projs) };
}

std::array<Vector3f, 3> Axes(const Box&)
{
	return{
		Vector3f{ 1.f, 0.f, 0.f },
		Vector3f{ 0.f, 1.f, 0.f },
		Vector3f{ 0.f, 0.f, 1.f }
	};
}

LineSegment Project(const Box& box, Vector3f ax)
{
	//there are 4 possible pairs of points depending on the direction of ax
	Vector3f c = box.a, d = box.b;

	if (ax.x() * ax.y() < 0) //different x and y
		std::swap(c.y(), d.y());

	if (ax.x() * ax.z() < 0) //different x and z
		std::swap(c.z(), d.z());

	return{ std::min(ax.dot(c), ax.dot(d)),
		std::max(ax.dot(c), ax.dot(d)) };
}

//line segment intersection
template<>
bool Intersects(const LineSegment& a, const LineSegment& b)
{
	return b.first <= a.second && b.second >= a.first;
}

template<>
bool Intersects(const Box& a, const Box& b)
{
	for (const Vector3f& ax : Axes(a))
		if (!Intersects(Project(a, ax), Project(b, ax)))
			return false;
	return true;
}

#endif