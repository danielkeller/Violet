#ifndef MATH_HPP
#define MATH_HPP

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
	return (Matrix4f(4, 4) << RotSclInv
		, -RotSclInv * mat.block<3, 1>(0, 3) //translation
		, 0, 0, 0, 1).finished();
}

#endif
