#ifndef SHAPES_HPP
#define SHAPES_HPP

#include <array>

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

//Transform?
struct OBB
{
	OBB(const AlignedBox3f& aabb);
	OBB(const AlignedBox3f& aabb, const Matrix4f& xfrm);
	OBB(const OBB&, const OBB&);
	OBB(const OBB&, const OBB&, const Matrix3f&);
    OBB(Mesh::const_iterator begin, Mesh::const_iterator end);

	Matrix3f axes;
	Vector3f origin;
	float volume() const;
	float squaredVolume() const;
};

Matrix4f OBBMat(const OBB& obb);
Matrix4f InvOBBMat(const OBB& obb);
OBB MergeFace(const OBB&, const OBB&);

#endif
