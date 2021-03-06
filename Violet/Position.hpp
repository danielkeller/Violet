#ifndef POSITION_HPP
#define POSITION_HPP

#include <unordered_map>
#include "magic_ptr.hpp"
#include "Core/Object.hpp"
#include "Core/Component.hpp"

struct BinaryPersistTag;
class Persist;

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

	Matrix4f ToMatrix() const;

	bool operator==(const Transform& other) const;
	bool operator!=(const Transform& other) const;

	friend std::ostream & operator<<(std::ostream &os, const Transform& p);

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	using PersistCategory = BinaryPersistTag;
};

class Position : public Component
{
	struct ObjData;

public:
	Position();

	magic_ptr<Transform> operator[](Object obj);
	const Transform& Get(Object obj);
	void Set(Object obj, const Transform& t);
	void Watch(Object obj, magic_ptr<Transform> w);

	//void Add(Object obj);
	void Load(const Persist&);
	void Unload(const Persist&);
	bool Has(Object obj) const;
	void Save(Object obj, Persist&) const;
	void Remove(Object obj);

private:
	struct ObjData
	{
		Transform loc;
		magic_ptr<Transform> target;
		EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	};

	std::unordered_map<Object, ObjData, std::hash<Object>, std::equal_to<Object>,
		Eigen::aligned_allocator<std::pair<const Object, ObjData>>> data;

	accessor<Transform, Object> acc;
};

MAKE_PERSIST_TRAITS(Position, Object, Transform)

#endif