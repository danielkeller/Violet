#include "stdafx.h"
#include "Shapes.hpp"

#include "Containers/WrappedIterator.hpp"
#include <numeric>
#include "Eigen/Eigenvalues"

//how thick flat things are
static const float ZERO_SIZE = 0.001f;

Vector3f centroid(const Mesh & m)
{
	auto rCenter = MapRange(m, static_cast<Vector3f(*)(const Triangle&)>(centroid));
	return
		std::accumulate(rCenter.begin(), rCenter.end(), Vector3f(0, 0, 0))
		/ float(m.size());
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
	return axes.colwise().norm().prod();
}

float OBB::squaredVolume() const
{
	return axes.colwise().squaredNorm().prod();
}

Matrix4f OBBMat(const OBB& obb)
{
	return (Matrix4f() << obb.axes, obb.origin, Vector4f{ 0,0,0,1 }.transpose()).finished();
}

Matrix4f InvOBBMat(const OBB& obb)
{
	Matrix3f RotSclInv = RotationScaleInverse(obb.axes);
	return (Matrix4f(4, 4) << RotSclInv
		, -RotSclInv * obb.origin //translation
		, 0, 0, 0, 1).finished();
}

Matrix3f InertiaTensor(const OBB& obb, Vector3f centerOfMass)
{
	Matrix3f ret = Matrix3f::Zero();

	auto addPt = [&](const Vector3f& boxPos)
	{
		Vector3f lpos = obb.origin + obb.axes * boxPos - centerOfMass;

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

OBB::OBB(const AlignedBox3f& aabb)
	: axes(Eigen::DiagonalMatrix<float, 3, 3>{aabb.sizes()}.toDenseMatrix())
	, origin(aabb.min())
{}

OBB::OBB(const AlignedBox3f& aabb, const Matrix4f& xfrm)
	: OBB(aabb)
{
	axes = xfrm.block<3, 3>(0, 0) * axes;
	origin = xfrm.block<3, 1>(0, 3) + xfrm.block<3, 3>(0, 0)*origin;
}

//newAxes must be orthogonal
OBB::OBB(const OBB& l, const OBB& r, const Matrix3f& newAxes)
{
	Matrix3f invAxes = newAxes.transpose();
	Matrix3f ourl = newAxes.transpose() * l.axes;
	Matrix3f ourr = newAxes.transpose() * r.axes;
	Vector3f lOrig = newAxes.transpose() * l.origin;
	Vector3f rOrig = newAxes.transpose() * r.origin;

	Vector3f lMax = ourl.cwiseMax(0).rowwise().sum() + lOrig;
	Vector3f rMax = ourr.cwiseMax(0).rowwise().sum() + rOrig;
	Vector3f lMin = ourl.cwiseMin(0).rowwise().sum() + lOrig;
	Vector3f rMin = ourr.cwiseMin(0).rowwise().sum() + rOrig;

	origin = newAxes * lMin.cwiseMin(rMin);
	axes = newAxes * Eigen::AlignedScaling3f{ lMax.cwiseMax(rMax) - lMin.cwiseMin(rMin) };
}

OBB::OBB(const OBB& l, const OBB& r)
{
	float lMass = l.volume(), rMass = r.volume();
	Vector3f lCenter = l.origin + l.axes*Vector3f{ .5f, .5f, .5f };
	Vector3f rCenter = r.origin + r.axes*Vector3f{ .5f, .5f, .5f };

	Vector3f centerOfMass = (lCenter*lMass + rCenter*rMass) / (lMass + rMass);

	Eigen::SelfAdjointEigenSolver<Matrix3f> es;

	Matrix3f vertInertiaTensor = InertiaTensor(l, centerOfMass)
		+ InertiaTensor(r, centerOfMass);

	es.computeDirect(vertInertiaTensor); //faster than compute()

	*this = { l, r, es.eigenvectors() };
}

OBB::OBB(const Mesh& m)
{
	if (m.size() == 0)
	{
		axes = -ZERO_SIZE * Matrix3f::Identity(); //make it inside out (i guess)
		origin = Vector3f::Zero();
		return;
	}

	Vector3f centerOfMass = centroid(m);
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

	for (const auto& tri : m)
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

	for (const auto& tri : m)
	{
		min = min.cwiseMin((axes.transpose() * tri).rowwise().minCoeff());
		max = max.cwiseMax((axes.transpose() * tri).rowwise().maxCoeff());
	}

	origin = axes * min;

	Vector3f extents = (max - min).cwiseMax(ZERO_SIZE);
	axes *= Eigen::AlignedScaling3f{ extents };
}
