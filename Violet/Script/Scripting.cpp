#include "stdafx.h"
#include "Scripting.hpp"

#include "Utils/Profiling.hpp"

#include "lua/src/lua.hpp"

template<class Class, int (Class::* fn)(lua_State *)>
int luaU_mem_fn(lua_State* L)
{
	void* classptr = lua_touserdata(L, lua_upvalueindex(1));
	Class* c = reinterpret_cast<Class*>(classptr);
	return (c->*fn)(L);
}

template<class Class, int (Class::* fn)(lua_State *)>
void luaU_push_mem_fn(lua_State* L, Class* c)
{
	lua_pushlightuserdata(L, c);
	lua_pushcclosure(L, &luaU_mem_fn<Class, fn>, 1);
}

// 'require's the function on top of the stack, and put it into global
// leaves module on top of stack
void luaU_require(lua_State* L, const char* modname, int glb)
{
	luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
	lua_getfield(L, -1, modname);  /* _LOADED[modname] */
	if (!lua_toboolean(L, -1))
	{  /* package not already loaded? */
		lua_pop(L, 1);  /* remove field */
		lua_pushvalue(L, -2); /*push loader*/
		lua_pushstring(L, modname);  /* argument to open function */
		lua_call(L, 1, 1);  /* call loader to open module */
		lua_pushvalue(L, -1);  /* make copy of module (call result) */
		lua_setfield(L, -3, modname);  /* _LOADED[modname] = module */
	}
	lua_remove(L, -2);  /* remove _LOADED table */
	if (glb)
	{
		lua_pushvalue(L, -1);  /* copy of module */
		lua_setglobal(L, modname);  /* _G[modname] = module */
	}
}

std::string luaU_checkstdstring(lua_State* L, int arg)
{
	size_t len;
	const char* str = luaL_checklstring(L, arg, &len);
	return{ str, str + len };
}

#define GAMELIBNAME "game"

//use a subset of lua's standard libs
static const luaL_Reg loadedlibs[] = {
	{ "_G", luaopen_base },
	{ LUA_LOADLIBNAME, luaopen_package },
	{ LUA_TABLIBNAME, luaopen_table },
	{ LUA_MATHLIBNAME, luaopen_math },
	{ NULL, NULL }
};

Scripting::Scripting(ComponentManager& mgr)
	: mgr(mgr), L(luaL_newstate())
{
	if (!L)
		throw std::runtime_error("Out of memory");

	//don't search random directories for scripts
	lua_pushboolean(L, true);
	lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");

	//"require" functions from 'loadedlibs'
	for (auto lib = loadedlibs; lib->func; lib++) {
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);  /* remove lib */
	}

	luaU_push_mem_fn<Scripting, &Scripting::OpenLib>(L, this);
	luaU_require(L, GAMELIBNAME, 1);

	if (luaL_dofile(L, "scripts/init.lua") != LUA_OK)
	{
		const char* msg = lua_tostring(L, -1);
		lua_pop(L, 1);
		throw std::runtime_error(msg);
	}
}

Scripting::~Scripting()
{
	lua_close(L);
}

int Scripting::OpenLib(lua_State* L)
{
	luaL_checkversion(L);
	lua_pushglobaltable(L);
	luaU_push_mem_fn<Scripting, &Scripting::Component>(L, this);
	lua_setfield(L, -2, "component");
	return 1;
}

int Scripting::Component(lua_State* L)
{
	components.emplace_back(std::make_unique<ScriptComponent>(luaU_checkstdstring(L, 1)));
	mgr.Register(components.back().get());

	lua_getglobal(L, "require");
	lua_pushvalue(L, -2); //hand it our argument
	lua_call(L, 1, 1); //require the package
	return 1;
}

void Scripting::PhysTick()
{
	luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
	for (const auto& c : components)
	{
		lua_getfield(L, -1, c->name.c_str());
		lua_getfield(L, -1, "tick");
		if (!lua_isnil(L, -1))
			lua_call(L, 0, 0);
		else
			lua_pop(L, 1);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}


