#ifndef POSITION_HPP
#define POSITION_HPP

#include <unordered_map>
#include "magic_ptr.hpp"
#include "Object.hpp"

struct Transform
{
	Vector3f pos;
	Quaternionf rot;
	float scale;
	Transform()
		: pos(0, 0, 0)
		, rot(Quaternionf::Identity())
		, scale(1)
	{}

	Matrix4f ToMatrix() const
	{
		return (Eigen::Translation3f(pos) * rot * Eigen::Scaling(scale)).matrix();
	}

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class Position
{
	struct ObjData;

public:
	magic_ptr<Transform>& operator[](Object obj);

private:
	struct ObjData
	{
		Transform loc;
		magic_ptr<Transform> target;
		EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	};

	std::unordered_map<Object, ObjData, std::hash<Object>, std::equal_to<Object>,
		Eigen::aligned_allocator<std::pair<Object, ObjData>>> data;
};

#endif