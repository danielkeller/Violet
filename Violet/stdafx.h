//Big headers go here

//keep gl from including windows.h garbage
#if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#define APIENTRY __stdcall
#endif

#include "GL/gl_core_3_3.h"
#include "Eigen/Dense"
#include "Eigen/Geometry"

using Eigen::Matrix3f;
using Eigen::Matrix4f;
using Eigen::Vector2f;
using Eigen::Vector3f;
using Eigen::Vector4f;

#define GLFW_INCLUDE_NONE