#ifndef MOBILE_HPP
#define MOBILE_HPP

#include "Containers/l_unordered_map.hpp"
#include "Position.hpp"

class Mobile
{
public:
	Mobile(Position& p) : position(p) {}
	void Update(float alpha);
	void Tick();

	magic_ptr<Matrix4f>& operator[](Object obj);

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
		Transform before;
		Transform loc;
		magic_ptr<Matrix4f> target; //but this uses lots of heap stuff and we want to avoid that?
		EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	};

	static Matrix4f interp(const Transform& before, const Transform& loc, float alpha);

	Position& position;
	// considering auto-picking allocators based on a type tag
	l_unordered_map<Object, ObjData, std::hash<Object>, std::equal_to<Object>,
	Eigen::aligned_allocator<std::pair<const Object, ObjData>>> data;
	//It's an exception anyway
	Transform cameraBefore;
	Transform cameraLoc;
	Matrix4f cameraMat;
};

#endif
