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

#define GLFW_INCLUDE_NONE

//enable stack traces on GL errors
#define GL_DEBUG

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

#include <utility>
//combine std and :: overload set for swap
using std::swap;
