#ifndef MOBILE_HPP
#define MOBILE_HPP

#include "Rendering/Render.hpp"
#include "Object.hpp"
#include "Permavector.hpp"

#include <map>

static const float dt = 0.03f;

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

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class Mobile
{
	struct ObjData;
	using PermaRef = Permavector<ObjData, Eigen::aligned_allocator<ObjData>>::perma_ref;

public:
	class ObjData;
	void Update(float alpha);
	void Tick();

	//A thing that we can move the object with
	class MoveProxy
	{
	public:
		Transform& operator*();
		Transform* operator->() { return &**this; }
		void Sync();
	private:
		PermaRef ref;
		Mobile& mobile;
		MoveProxy(PermaRef, Mobile&);
		friend class Mobile;
	};

	MoveProxy Add(const Transform& loc, Render::LocationProxy target);

private:
	struct ObjData
	{
		Render::LocationProxy target;
		Transform before;
		Transform loc;
		EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	};

	Permavector<ObjData, Eigen::aligned_allocator<ObjData>> data;
};

#endif