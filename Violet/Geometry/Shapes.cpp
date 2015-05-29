#include "stdafx.h"
#include "Shapes.hpp"

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
