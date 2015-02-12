#ifndef POSITION_HPP
#define POSITION_HPP

#include "Rendering/Render.hpp"
#include <unordered_map>

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
	struct TransformProxy
	{
		void operator=(const Transform&);
		operator Transform();
	private:
		ObjData* data;
		TransformProxy(ObjData*);
		friend class Position;
	};

	TransformProxy operator[](Object obj);
	void Watch(Object obj, Render::LocationProxy proxy);

private:
	struct ObjData
	{
		std::vector<Render::LocationProxy> targets;
		Transform loc;
		EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	};

	std::unordered_map<Object, ObjData> data;
};

#endif