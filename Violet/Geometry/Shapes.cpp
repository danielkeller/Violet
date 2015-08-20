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
	return extent.prod() * 8.f;
}

Matrix4f OBB::matrix() const
{
    Matrix3f halfBox = axes * Eigen::AlignedScaling3f{ extent };
    return (Matrix4f() << halfBox * 2.f, origin - halfBox.rowwise().sum(), Vector4f{ 0,0,0,1 }.transpose()).finished();
}

Matrix3f InertiaTensor(const OBB& obb, Vector3f centerOfMass)
{
	Matrix3f ret = Matrix3f::Zero();

	auto addPt = [&](const Vector3f& boxPos)
	{
		Vector3f lpos = obb.origin + 2 * Eigen::DiagonalMatrix<float, 3>{obb.extent} * obb.axes * boxPos - centerOfMass;

		ret(0, 0) += (lpos.y()*lpos.y() + lpos.z()*lpos.z());
		ret(1, 1) += (lpos.x()*lpos.x() + lpos.z()*lpos.z());
		ret(2, 2) += (lpos.x()*lpos.x() + lpos.y()*lpos.y());
		ret(1, 0) -= lpos.x()*lpos.y();
		ret(2, 0) -= lpos.x()*lpos.z();
		ret(2, 1) -= lpos.y()*lpos.z();
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
    obb.axes = xfrm.rot.matrix() * obb.axes;
    obb.origin = xfrm.pos + xfrm.rot.matrix() * obb.origin;
    obb.extent *= xfrm.scale;
    return obb;
}

AlignedBox3f OBB::Bound() const
{
    Vector3f diag = axes.cwiseAbs() * extent;
    return { origin - diag, origin + diag};
}

OBB::OBB(const AlignedBox3f& aabb)
    : axes(Matrix3f::Identity())
	, origin(aabb.center())
    , extent(aabb.sizes() / 2.f)
{}

OBB::OBB(Mesh::const_iterator begin, Mesh::const_iterator end)
{
	if (begin == end)
	{
		axes = -ZERO_SIZE * Matrix3f::Identity(); //make it inside out (i guess)
		origin = Vector3f::Zero();
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
	axes = es.eigenvectors();

	float maxflt = std::numeric_limits<float>::max();
	Eigen::Vector3f min{ maxflt, maxflt, maxflt };
	Eigen::Vector3f max = -min;

	for (const auto& tri : make_range(begin, end))
	{
		min = min.cwiseMin((axes.transpose() * tri).rowwise().minCoeff());
		max = max.cwiseMax((axes.transpose() * tri).rowwise().maxCoeff());
	}

    extent = (max - min).cwiseMax(ZERO_SIZE) / 2.f;
    origin = axes * (min + extent);
}
