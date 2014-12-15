//Big headers go here

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
//#define GL_DEBUG
