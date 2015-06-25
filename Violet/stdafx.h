//Big headers go here
//TODO: split into gl, math, etc

#ifndef NDEBUG
//Disable some of MSVC's more unnecesary checks
#define _ITERATOR_DEBUG_LEVEL 1
#endif

#ifdef _WIN32
#define APIENTRY __stdcall
#endif

#ifndef _WIN32
#define _POSIX_C_SOURCE 200112L
#endif

#include "GL/gl_core_3_3.h"
#include "Eigen/Core"
#include "Eigen/Geometry"
#include <vector>
#include <memory>
#include <string>

using Eigen::Vector2i;
using Eigen::Matrix3f;
using Eigen::Matrix4f;
using Eigen::Vector2f;
using Eigen::Vector3f;
using Eigen::Vector4f;
using Eigen::Quaternionf;

#define PI_F 3.14159265358979f

//#define _GLIBCXX_DEBUG

#define GLFW_INCLUDE_NONE

//enable stack traces on GL errors
#if !defined(__APPLE__) && !defined(_WIN32)
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

#define POD_EQUALITY(Class) \
	bool operator==(const Class& other) const \
	{ return std::memcmp(this, &other, sizeof(Class)) == 0; }\
	bool operator!=(const Class& other) const \
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

#define MAKE_PERSIST_TRAITS(Class, Key, ...) \
	template<class Subsystem> struct PersistTraits;\
	template<> struct PersistTraits<Class> { \
		using key = Key; \
		using data = std::tuple<Key, ##__VA_ARGS__>; };

#define EXCEPT_INFO_BEGIN try {
#define EXCEPT_INFO_END(str) } catch (std::runtime_error& err) { \
	throw std::runtime_error(std::string("for ") + str + ": " + err.what());} \
	catch (std::logic_error& err) { \
	throw std::logic_error(std::string("for ") + str + ": " + err.what());}

//for static initialization code:

//runs following block only once
#define STATIC for (static bool STATIC_inited = false; !STATIC_inited; STATIC_inited = true)

#include <utility>
//combine std and :: overload set for swap
using std::swap;
using std::to_string;

inline bool ends_with(std::string const & value, std::string const & ending)
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

inline std::size_t operator "" _sz(unsigned long long int x)
{
	return static_cast<std::size_t>(x);
}
