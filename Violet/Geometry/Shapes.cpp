#include "stdafx.h"
#include "Shapes.hpp"

#include "Containers/WrappedIterator.hpp"
#include "Utils/Math.hpp"

#include "Position.hpp"

#include <numeric>
#include "Eigen/Eigenvalues"

//how thick flat things are
static const float ZERO_SIZE = 0.001f;

Vector3f centroid(const Mesh & m)
{
    return centroid(m.begin(), m.end());
}

Vector3f centroid(Mesh::const_iterator begin, Mesh::const_iterator end)
{
    Vector3f sum = std::accumulate(begin, end, Vector3f{0, 0, 0},
    [] (const Vector3f& v, const Triangle& t) -> Vector3f
    {
        return v + centroid(t);
    });
    
    return sum / (end - begin);
}

Triangle TransformTri(const Triangle& t, const Matrix4f& mat)
{
	Eigen::Matrix<float, 4, 3> triExt;
	triExt << t, 1, 1, 1;
	return (mat * triExt).block<3, 3>(0, 0);
}

Matrix4f BoxMat(const AlignedBox3f& box)
{
	return Eigen::Affine3f{ Eigen::Translation3f{ box.min() }
		*Eigen::AlignedScaling3f{ box.max() - box.min() }
	}.matrix();
}

Matrix4f InvBoxMat(const AlignedBox3f& box)
{
	return Eigen::Affine3f{
		Eigen::AlignedScaling3f{ (box.max() - box.min()).cwiseInverse() }
		*Eigen::Translation3f{ -box.min() }
	}.matrix();
}

AlignedBox3f Bound(const Mesh& m)
{
	float maxflt = std::numeric_limits<float>::max();
	Eigen::Vector3f min{ maxflt, maxflt, maxflt };
	Eigen::Vector3f max = -min;

	for (const auto& tri : m)
	{
		min = min.cwiseMin(tri.rowwise().minCoeff());
		max = max.cwiseMax(tri.rowwise().maxCoeff());
	}

	return{ min, max };
}

float OBB::volume() const
{
	return product(extent) * 8.f;
}
/*
Matrix3 OBB::matrix() const
{
    Matrix3 halfBox = axes.scale(extent);
    return affine(halfBox * 2, origin - halfBox.col[0] - halfBox.col[1] - halfBox.col[2]);
}*/

Matrix3f InertiaTensor(const OBB& obb, Vector3 centerOfMass)
{
    Matrix3f ret = Matrix3f::Zero();

	auto addPt = [&](const Vector3& boxPos)
	{
		Vector3 lpos = obb.origin + obb.axes.scale(obb.extent) * boxPos * 2 - centerOfMass;

		ret(0, 0) += (lpos.y*lpos.y + lpos.z*lpos.z);
		ret(1, 1) += (lpos.x*lpos.x + lpos.z*lpos.z);
		ret(2, 2) += (lpos.x*lpos.x + lpos.y*lpos.y);
		ret(1, 0) -= lpos.x*lpos.y;
		ret(2, 0) -= lpos.x*lpos.z;
		ret(2, 1) -= lpos.y*lpos.z;
	};

	//box faces
	addPt({ .5f, .5f, 0 });
	addPt({ 0, .5f, .5f });
	addPt({ 0.5f, 0, .5f });
	addPt({ .5f, .5f, 1 });
	addPt({ 1, .5f, .5f });
	addPt({ 0.5f, 1, .5f });

	return ret * obb.volume(); //mass
}

OBB operator*(const Transform& xfrm, OBB obb)
{
    Quat q{xfrm.rot.w(), xfrm.rot.x(), xfrm.rot.y(), xfrm.rot.z()};
    Matrix3 mq = q.matrix();
    obb.axes = mq * obb.axes;
    Vector3 p{xfrm.pos.x(), xfrm.pos.y(), xfrm.pos.z()};
    obb.origin = p + mq * obb.origin;
    obb.extent *= xfrm.scale;
    return obb;
}

Box3 OBB::Bound() const
{
    Vector3 diag = abs(axes) * extent;
    return { origin - diag, origin + diag};
}

OBB::OBB(const Box3& aabb)
    : axes(Identity())
	, origin(aabb.center())
    , extent(aabb.size() / 2)
{}

OBB::OBB(Mesh::const_iterator begin, Mesh::const_iterator end)
{
	if (begin == end)
	{
		axes = -ZERO_SIZE * Identity(); //make it inside out (i guess)
		origin = 0;
		return;
	}

	Vector3f centerOfMass = centroid(begin, end);
	Matrix3f inertiaTensor = Matrix3f::Zero();

	auto addPt = [&](const Vector3f& pt, float mass)
	{
		Vector3f lpos = pt - centerOfMass;

		inertiaTensor(0, 0) += (lpos.y()*lpos.y() + lpos.z()*lpos.z()) * mass;
		inertiaTensor(1, 1) += (lpos.x()*lpos.x() + lpos.z()*lpos.z()) * mass;
		inertiaTensor(2, 2) += (lpos.x()*lpos.x() + lpos.y()*lpos.y()) * mass;
		inertiaTensor(1, 0) -= lpos.x()*lpos.y() * mass;
		inertiaTensor(2, 0) -= lpos.x()*lpos.z() * mass;
		inertiaTensor(2, 1) -= lpos.y()*lpos.z() * mass;
	};

	for (const auto& tri : make_range(begin, end))
	{
		float area = TriNormal(tri).norm() / 6.f;
		addPt(tri.col(0), area);
		addPt(tri.col(1), area);
		addPt(tri.col(2), area);
	}

	Eigen::SelfAdjointEigenSolver<Matrix3f> es;
	es.computeDirect(inertiaTensor);
    axes = ToM(es.eigenvectors());

	float maxflt = std::numeric_limits<float>::max();
	Vector3 bot{ maxflt, maxflt, maxflt };
	Vector3 top = -bot;

	for (const auto& tri : make_range(begin, end))
	{
        Matrix3 triM = Transpose3(axes) * ToM(tri);
        bot = min(bot, min(triM.col));
        top = max(top, max(triM.col));
	}

    extent = max((top - bot), Vector3{ZERO_SIZE, ZERO_SIZE, ZERO_SIZE}) / 2.f;
    origin = axes * (bot + extent);
}
