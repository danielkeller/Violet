#ifndef SHAPES_HPP
#define SHAPES_HPP

#include <array>

using LineSegment = std::pair<float, float>;

struct Triangle
{
	Vector3f q, r, s;
};

using Eigen::AlignedBox3f;

#endif
