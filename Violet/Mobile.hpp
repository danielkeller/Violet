#ifndef MOBILE_HPP
#define MOBILE_HPP

#include "Rendering/Render.hpp"
#include "Object.hpp"
#include "Containers/l_bag.hpp"

#include "Position.hpp"

#include <map>

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
        const Transform& operator*() const;
        Transform* operator->() { return &**this; }
        const Transform* operator->() const { return &**this; }
		void Add(Render::LocationProxy target);
		void Remove(Render::LocationProxy target);
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
	Matrix4f CameraMat() const
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
