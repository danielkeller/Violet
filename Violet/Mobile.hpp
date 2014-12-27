#ifndef MOBILE_HPP
#define MOBILE_HPP

#include "Rendering/Render.hpp"
#include "Object.hpp"
#include "Containers/l_bag.hpp"

#include <map>

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
	using PermaRef = l_bag<ObjData, Eigen::aligned_allocator<ObjData>>::perma_ref;

public:
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

	MoveProxy Create(const Transform& loc, std::vector<Render::LocationProxy> targets);

	Transform& CameraLoc()
	{
		return cameraLoc;
	}
	Matrix4f CameraMat()
	{
		return cameraMat;
	}

private:
	struct ObjData
	{
		std::vector<Render::LocationProxy> targets;
		Transform before;
		Transform loc;
		EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	};

	static Matrix4f interp(const Transform& before, const Transform& loc, float alpha);

	l_bag<ObjData, Eigen::aligned_allocator<ObjData>> data;
	//It's an exception anyway
	Transform cameraBefore;
	Transform cameraLoc;
	Matrix4f cameraMat;
};

#endif
