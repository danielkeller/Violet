#ifndef SHAPES_HPP
#define SHAPES_HPP

#include <array>

#include "Geometry/Vector.hpp"

using LineSegment = std::pair<float, float>;

using Triangle = Eigen::Matrix3f;

inline Vector3f centroid(const Triangle& t)
{
	return t.rowwise().sum() / 3.f;
}

Triangle TransformTri(const Triangle& t, const Matrix4f& mat);

//result is not neccesarily normalized
inline Vector3f TriNormal(const Triangle& t)
{
	return (t.col(1) - t.col(0)).cross(t.col(2) - t.col(0));
}

//you can't forward declare a type like this
using Mesh = std::vector<Triangle, Eigen::aligned_allocator<Triangle>>;

Vector3f centroid(const Mesh& m);
Vector3f centroid(Mesh::const_iterator begin, Mesh::const_iterator end);

using Eigen::AlignedBox3f;
Matrix4f BoxMat(const AlignedBox3f& box);
Matrix4f InvBoxMat(const AlignedBox3f& box);

struct Transform;

//Transform?
struct OBB
{
	OBB(const Box3& aabb);
    OBB(Mesh::const_iterator begin, Mesh::const_iterator end);

	Matrix3 axes;
	Vector3 origin;
    Vector3 extent;
	float volume() const;
    
    Box3 Bound() const;
    
    Matrix3 matrix() const;
};

OBB operator*(const Transform& xfrm, OBB obb);

OBB MergeFace(const OBB&, const OBB&);

#endif
