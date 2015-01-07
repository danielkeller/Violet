//Big headers go here
//TODO: split into gl, math, etc

#include "GL/gl_core_3_3.h"
#include "Eigen/Dense"
#include "Eigen/Geometry"
//prevent vectors from breaking things
#include "Eigen/StdVector"

using Eigen::Matrix3f;
using Eigen::Matrix4f;
using Eigen::Vector2f;
using Eigen::Vector3f;
using Eigen::Vector4f;
using Eigen::Quaternionf;
using vectorVector3f = std::vector<Vector3f, Eigen::aligned_allocator<Vector3f>>;

#define PI 3.141592653589793238463
#define PI_F 3.14159265358979f

//#define _GLIBCXX_DEBUG

#define GLFW_INCLUDE_NONE

//enable stack traces on GL errors
#ifndef __APPLE__
#define GL_DEBUG
#endif

#define BASIC_EQUALITY(Class, Member) \
	bool operator==(const Class& other) const \
	{ return Member == other.Member; }\
	bool operator!=(const Class& other) const \
	{ return !(*this == other); }

#define MEMBER_EQUALITY(MemberTy, Member) \
	bool operator==(const MemberTy& other) const \
	{ return Member == other; }\
	bool operator!=(const MemberTy& other) const \
	{ return !(*this == other); }

#define HAS_HASH template<class T> friend struct std::hash;
#define MEMBER_HASH(type, member) template<> struct std::hash<type> \
    { size_t operator()(const type& v) const\
        { return std::hash<decltype(type::member)>()(v.member);}};
#define HASH_DECL(thing, type) thing type; template<> struct std::hash<type> \
    { size_t operator()(const type& v) const; };
#define HASH_DEFN(type, member) template<> size_t std::hash<type>:: \
    operator()(const type& v) const\
        { return std::hash<decltype(type::member)>()(v.member);}

#include <utility>
//combine std and :: overload set for swap
using std::swap;
using std::to_string;

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}
