#include "stdafx.h"

#include "Collide.hpp"

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
	const Vector3f
		a = t.col(0) - t.col(1),
		b = t.col(1) - t.col(2),
		c = t.col(2) - t.col(0);
	Vector3f face = a.cross(b).normalized();
	return{
		face, //If I put braces around this the compiler crashes
		{ face.cross(a).normalized() },
		{ face.cross(b).normalized() },
		{ face.cross(c).normalized() }
	};
}

LineSegment Project(const Triangle& t, Vector3f ax)
{
	Vector3f projs = t.matrix().transpose() * ax;
	return{ projs.minCoeff(), projs.maxCoeff() };
}

std::array<Vector3f, 3> Axes(const AlignedBox3f&)
{
	return{
		Vector3f{ 1.f, 0.f, 0.f },
		Vector3f{ 0.f, 1.f, 0.f },
		Vector3f{ 0.f, 0.f, 1.f }
	};
}

LineSegment Project(const AlignedBox3f& box, Vector3f ax)
{
	//there are 4 possible pairs of points depending on the direction of ax
	Vector3f c = box.min(), d = box.max();

	if (ax.x() * ax.y() < 0) //different x and y
		std::swap(c.y(), d.y());

	if (ax.x() * ax.z() < 0) //different x and z
		std::swap(c.z(), d.z());

	return{ std::min(ax.dot(c), ax.dot(d)),
		std::max(ax.dot(c), ax.dot(d)) };
}

bool ConservativeIntersects(const AlignedBox3f& a, const Triangle& tri)
{
	//all points (col), on any axis (row), are < min or > max
	return !(
		(tri.array() < a.min().array().replicate<1, 3>())
		.rowwise().all()
		|| (tri.array() > a.max().array().replicate<1, 3>())
		.rowwise().all()
		).any();
}

//See Real Time Collision Detection p 102
bool ConservativeOBBvsOBB(const OBB& l, const OBB& r)
{
    Matrix3f rot = l.axes * r.axes.transpose();
    Matrix3f absRot = rot.cwiseAbs();
    Vector3f dist = l.axes * (r.origin - l.origin);
    
    //They don't overlap if on some axis the distance between them is less than
    //the sum of the radii
    
    //l's axes
    if (((l.extent + absRot * r.extent).array() < dist.cwiseAbs().array()).any())
        return false;
    
    Vector3f dist1 = rot.transpose() * dist;
    //r's axes
    if (((absRot.transpose() * l.extent + r.extent).array() < dist1.cwiseAbs().array()).any())
        return false;
    
    return true;
}

std::tuple<Vector3f, Vector3f, bool> TriPlaneIntersect(
	const Vector3f& pOrig, const Vector3f& pNorm, const Triangle& tri)
{
	Triangle triTransl = tri.colwise() - pOrig;

	//which side is each point on?
	Eigen::Array<bool, 3, 1> sides = (triTransl.transpose() * pNorm).array() > 0;

	Vector3f::Index same1, same2, opposite;

	if (sides[0] == sides[1])
	{
		if (sides[1] == sides[2])
			return std::make_tuple(Vector3f{}, Vector3f{}, false);
		//2 is opposite 0 and 1
		same1 = 0; same2 = 1; opposite = 2;
	}
	else
	{
		if (sides[1] == sides[2])
		{
			same1 = 1; same2 = 2; opposite = 0;
		}
		else
		{
			same1 = 0; same2 = 2; opposite = 1;
		}
	}

	//line-plane intersection
	Vector3f int1 = tri.col(opposite) + (tri.col(same1) - tri.col(opposite)) *
		(pOrig - tri.col(opposite)).dot(pNorm) / (tri.col(same1) - tri.col(opposite)).dot(pNorm);
	Vector3f int2 = tri.col(opposite) + (tri.col(same2) - tri.col(opposite)) *
		(pOrig - tri.col(opposite)).dot(pNorm) / (tri.col(same2) - tri.col(opposite)).dot(pNorm);

	return std::make_tuple(int1, int2, true);
}

std::pair<Vector3f, bool> ContactPoint(const Triangle& l, const Triangle& r)
{
	Vector3f pt1, pt2, pt3, pt4;
	bool intersection;

	Vector3f rNorm = TriNormal(r),
		lNorm = TriNormal(l);

	//handle degenerate triangles
	if (rNorm.isZero() && lNorm.isZero())
		return{ {}, false };
	else if (rNorm.isZero())
	{
		std::tie(pt1, pt2, intersection) = TriPlaneIntersect(l.col(0), lNorm, r); //still works
		return{ (pt1 + pt2) / 2.f, intersection };
	}
	else if (lNorm.isZero())
	{
		std::tie(pt3, pt4, intersection) = TriPlaneIntersect(r.col(0), rNorm, l);
		return{ (pt3 + pt4) / 2.f, intersection };
	}

	std::tie(pt1, pt2, intersection) = TriPlaneIntersect(l.col(0), lNorm, r);

	if (!intersection)
		return{ {}, false };

	std::tie(pt3, pt4, intersection) = TriPlaneIntersect(r.col(0), rNorm, l);

	if (!intersection)
		return{ {}, false };

	//now find the middle points
	Vector3f::Index sortDir;
	(pt2 - pt1).cwiseAbs().maxCoeff(&sortDir);
	float top = std::max(pt1[sortDir], pt2[sortDir]);
	float bottom = std::min(pt1[sortDir], pt2[sortDir]);

	if ((pt3[sortDir] > top && pt4[sortDir] > top)
		|| (pt3[sortDir] < bottom && pt4[sortDir] < bottom))
		return{ {}, false }; //line segments don't intersect

	//4-element sorting network from Wikipedia (minus last swap)
#define CHECK(n, m) if (pt##n[sortDir] > pt##m[sortDir]) swap(pt##n, pt##m)
	CHECK(1, 3);
	CHECK(2, 4);
	CHECK(1, 2);
	CHECK(3, 4);

	return{ (pt2 + pt3) / 2.f, true };
}
