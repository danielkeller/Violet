#ifndef SHAPES_HPP
#define SHAPES_HPP

#include <array>

using LineSegment = std::pair<float, float>;

using Triangle = Eigen::Array33f;

inline Vector3f centroid(const Triangle& t)
{
	return t.rowwise().sum() / 3.f;
}

using Eigen::AlignedBox3f;
Matrix4f BoxMat(const AlignedBox3f& box);
Matrix4f InvBoxMat(const AlignedBox3f& box);

//Transform?
struct OBB
{
	Matrix3f axes;
	Vector3f origin;
	float volume() const;
};

Matrix4f OBBMat(const OBB& obb);
Matrix4f InvOBBMat(const OBB& obb);
OBB AABBToObb(const AlignedBox3f& aabb);
OBB AABBToObb(const AlignedBox3f& aabb, const Matrix4f& xfrm);
OBB Merge(const OBB&, const OBB&);
OBB MergeFace(const OBB&, const OBB&);

//TODO: some kind of math header
template<class Derived>
auto RotationScaleInverse(const Eigen::MatrixBase<Derived>& mat)
{
	return (
		mat.array().rowwise() / mat.colwise().squaredNorm().array() //scaling
		).transpose(); //rotation
}

template<class Derived>
Matrix4f AffineInverse(const Eigen::MatrixBase<Derived>& mat)
{
	Matrix3f RotSclInv = RotationScaleInverse(mat.block<3, 3>(0, 0));
	return (Matrix4f(4,4) << RotSclInv
		, -RotSclInv * mat.block<3, 1>(0, 3) //translation
		, 0, 0, 0, 1).finished();
}

#endif
