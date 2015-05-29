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

//TODO: some kind of math header
template<class Derived>
Matrix4f AffineInverse(const Eigen::MatrixBase<Derived>& mat)
{
	Matrix3f RotSclInv = (
		mat.block<3, 3>(0, 0).array().rowwise()
		/ mat.block<3, 3>(0, 0).colwise().squaredNorm().array() //scaling
		).transpose(); //rotation
	return (Matrix4f(4,4) << RotSclInv
		, -RotSclInv * mat.block<3, 1>(0, 3) //translation
		, 0, 0, 0, 1).finished();
}

#endif
