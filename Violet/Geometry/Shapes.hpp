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

#endif
