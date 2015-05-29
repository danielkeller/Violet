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
		(tri < a.min().array().replicate<1, 3>())
		.rowwise().all()
		|| (tri > a.max().array().replicate<1, 3>())
		.rowwise().all()
		).any();
}

bool ConservativeOBBvsOBB1(const Matrix4f& lInv, const Matrix4f& r)
{
	using Eigen::Array3f;
	Matrix4f rToL = lInv * r;
	Array3f pt = rToL.block<3, 1>(0, 3);
	Eigen::Array33f frame = rToL.block<3, 3>(0, 0);

	return (frame.cwiseMax(0).rowwise().sum() + pt > Array3f::Zero()).all() //all max > 0
		&& (frame.cwiseMin(0).rowwise().sum() + pt < Array3f::Ones()).all(); //all min < 1
}

bool ConservativeOBBvsOBB(const Matrix4f& l, const Matrix4f& lInv, const Matrix4f& r, const Matrix4f& rInv)
{
	return ConservativeOBBvsOBB1(lInv, r) && ConservativeOBBvsOBB1(rInv, l);
}
