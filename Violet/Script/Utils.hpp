#ifndef SCRIPT_UTILS_HPP
#define SCRIPT_UTILS_HPP

#include "lua/src/lua.h"
#include "lua/src/lualib.h"
#include "lua/src/lauxlib.h"

//wrap a member function. expects the class pointer as argument 1
template<class Class, int (Class::* fn)(lua_State *)>
int luaU_member(lua_State* L)
{
	void* classptr = lua_touserdata(L, -1);
	Class* c = reinterpret_cast<Class*>(classptr);
	return (c->*fn)(L);
}

//a lua analog of mem_fn: expects the class pointer as a userdata in slot 1
template<class Class, int (Class::* fn)(lua_State *)>
int luaU_mem_fn(lua_State* L)
{
	void* classptr = lua_touserdata(L, lua_upvalueindex(1));
	Class* c = reinterpret_cast<Class*>(classptr);
	return (c->*fn)(L);
}

//a lua analog of mem_fn: creates the function and the closure
template<class Class, int (Class::* fn)(lua_State *)>
void luaU_push_mem_fn(lua_State* L, Class* c)
{
	lua_pushlightuserdata(L, c);
	lua_pushcclosure(L, &luaU_mem_fn<Class, fn>, 1);
}

//call destructor on userdata in argument 1
template<class T>
int luaU_delete(lua_State* L)
{
	void* classptr = lua_touserdata(L, -1);
	reinterpret_cast<T*>(classptr)->~T();
	lua_pop(L, 1);
	return 0;
}

//metatable should be on top of stack. pops metatable, pushes object, and returns
//a reference. the reference will be valid until control returns to lua
template<class T, typename... Args>
T& luaU_new(lua_State* L, Args... args)
{
	void* ptr = lua_newuserdata(L, sizeof(T));
	//call ctor before lua_setmetatable so if ctor throws, __gc doesn't run
	T* tptr = new(ptr) T(args...);
	lua_pushvalue(L, -2); //copy metatable
	lua_setmetatable(L, -2);
	lua_remove(L, -2);
	return *tptr;
}

// 'require's the function on top of the stack, and put it into global
// leaves module on top of stack
void luaU_require(lua_State* L, const char* modname, int glb);
std::string luaU_checkstdstring(lua_State* L, int arg);

#endif
