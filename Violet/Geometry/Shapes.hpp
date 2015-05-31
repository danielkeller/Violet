#ifndef SHAPES_HPP
#define SHAPES_HPP

#include <array>

using LineSegment = std::pair<float, float>;

using Triangle = Eigen::Matrix3f;

inline Vector3f centroid(const Triangle& t)
{
	return t.rowwise().sum() / 3.f;
}

//you can't forward declare a type like this
using Mesh = std::vector<Triangle, Eigen::aligned_allocator<Triangle>>;

Vector3f centroid(const Mesh& m);

using Eigen::AlignedBox3f;
Matrix4f BoxMat(const AlignedBox3f& box);
Matrix4f InvBoxMat(const AlignedBox3f& box);

//Transform?
struct OBB
{
	OBB(const AlignedBox3f& aabb);
	OBB(const AlignedBox3f& aabb, const Matrix4f& xfrm);
	OBB(const OBB&, const OBB&);
	OBB(const OBB&, const OBB&, const Matrix3f&);
	OBB(const Mesh&);

	Matrix3f axes;
	Vector3f origin;
	float volume() const;
	float squaredVolume() const;
};

Matrix4f OBBMat(const OBB& obb);
Matrix4f InvOBBMat(const OBB& obb);
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
