#ifndef SHAPES_HPP
#define SHAPES_HPP

#include "stdafx.h"
#include <array>

using LineSegment = std::pair<float, float>;

struct Triangle
{
	Vector3f q, r, s;
};

struct Box
{
	Vector3f a, b;
};

#endif