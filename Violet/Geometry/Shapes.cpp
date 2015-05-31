#include "stdafx.h"
#include "Shapes.hpp"

#include "Eigen/Eigenvalues"

OBB AABBToObb(const AlignedBox3f& aabb)
{
	return{ Eigen::DiagonalMatrix<float, 3, 3>{aabb.sizes()}.toDenseMatrix(),
		aabb.min() };
}

OBB AABBToObb(const AlignedBox3f& aabb, const Matrix4f& xfrm)
{
	OBB ret = AABBToObb(aabb);
	return{ xfrm.block<3,3>(0,0) * ret.axes, xfrm.block<3,1>(0,3) + xfrm.block<3,3>(0,0)*ret.origin };
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

float OBB::volume() const
{
	return axes.colwise().norm().prod();
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
/*
OBB Merge(const OBB& l, const OBB& r)
{
	OBB ret;

	Quaternionf lRot{ l.axes.colwise().normalized() };
	Quaternionf rRot{ r.axes.colwise().normalized() };

	Matrix3f axes = lRot.slerp(0.5f, rRot).matrix();
	Matrix3f invAxes = axes.transpose();

	Vector3f lMax = (invAxes * l.axes).cwiseMax(0).rowwise().sum()
		+ invAxes * l.origin;
	Vector3f rMax = (invAxes * r.axes).cwiseMax(0).rowwise().sum()
		+ invAxes * r.origin;

	Vector3f lMin = (invAxes * l.axes).cwiseMin(0).rowwise().sum()
		+ invAxes * l.origin;
	Vector3f rMin = (invAxes * r.axes).cwiseMin(0).rowwise().sum()
		+ invAxes * r.origin;

	ret.origin = axes * lMin.cwiseMin(rMin);
	ret.axes = axes * Eigen::AlignedScaling3f{ lMax.cwiseMax(rMax) - lMin.cwiseMin(rMin) };
	return ret;
}
*/

Matrix3f VertexInertiaTensor(const OBB& obb, Vector3f centerOfMass)
{
	Matrix3f ret = Matrix3f::Zero();

	static AlignedBox3f box{ Vector3f::Zero(), Vector3f::Ones() };
	for (int corner = AlignedBox3f::BottomLeftFloor;
	corner <= AlignedBox3f::TopRightCeil; ++corner)
	{
		Vector3f lpos = obb.origin
			+ obb.axes * box.corner(static_cast<AlignedBox3f::CornerType>(corner))
			- centerOfMass;

		ret(0, 0) += (lpos.y()*lpos.y() + lpos.z()*lpos.z());
		ret(1, 1) += (lpos.x()*lpos.x() + lpos.z()*lpos.z());
		ret(2, 2) += (lpos.x()*lpos.x() + lpos.y()*lpos.y());
		ret(1, 0) -= lpos.x()*lpos.y();
		ret(2, 0) -= lpos.x()*lpos.z();
		ret(2, 1) -= lpos.y()*lpos.z();
	}

	return ret * obb.volume(); //mass
}


Matrix3f FaceInertiaTensor(const OBB& obb, Vector3f centerOfMass)
{
	Matrix3f ret = Matrix3f::Zero();

	auto addPt = [&](const Vector3f& boxPox)
	{
		Vector3f lpos = obb.origin + obb.axes * boxPox - centerOfMass;

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
//newAxes must be orthogonal
OBB Merge(const OBB& l, const OBB& r, const Matrix3f& newAxes)
{
	OBB ret;
	Matrix3f invAxes = newAxes.transpose();
	Matrix3f ourl = newAxes.transpose() * l.axes;
	Matrix3f ourr = newAxes.transpose() * r.axes;
	Vector3f lOrig = newAxes.transpose() * l.origin;
	Vector3f rOrig = newAxes.transpose() * r.origin;

	Vector3f lMax = ourl.cwiseMax(0).rowwise().sum() + lOrig;
	Vector3f rMax = ourr.cwiseMax(0).rowwise().sum() + rOrig;
	Vector3f lMin = ourl.cwiseMin(0).rowwise().sum() + lOrig;
	Vector3f rMin = ourr.cwiseMin(0).rowwise().sum() + rOrig;

	ret.origin = newAxes * lMin.cwiseMin(rMin);
	ret.axes = newAxes * Eigen::AlignedScaling3f{ lMax.cwiseMax(rMax) - lMin.cwiseMin(rMin) };
	return ret;
}

#include <iostream>

OBB Merge(const OBB& l, const OBB& r)
{
	float lMass = l.volume(), rMass = r.volume();
	Vector3f lCenter = l.origin + l.axes*Vector3f{ .5f, .5f, .5f };
	Vector3f rCenter = r.origin + r.axes*Vector3f{ .5f, .5f, .5f };

	Vector3f centerOfMass = (lCenter*lMass + rCenter*rMass) / (lMass + rMass);

	Eigen::SelfAdjointEigenSolver<Matrix3f> es;

	Matrix3f vertInertiaTensor = VertexInertiaTensor(l, centerOfMass)
		+ VertexInertiaTensor(r, centerOfMass);

	es.computeDirect(vertInertiaTensor); //faster than compute()

	return Merge(l, r, es.eigenvectors());
}

OBB MergeFace(const OBB& l, const OBB& r)
{
	float lMass = l.volume(), rMass = r.volume();
	Vector3f lCenter = l.origin + l.axes*Vector3f{ .5f, .5f, .5f };
	Vector3f rCenter = r.origin + r.axes*Vector3f{ .5f, .5f, .5f };

	Vector3f centerOfMass = (lCenter*lMass + rCenter*rMass) / (lMass + rMass);

	Eigen::SelfAdjointEigenSolver<Matrix3f> es;

	Matrix3f faceInertiaTensor = FaceInertiaTensor(l, centerOfMass)
		+ FaceInertiaTensor(r, centerOfMass);

	es.computeDirect(faceInertiaTensor); //faster than compute()

	return Merge(l, r, es.eigenvectors());
}
