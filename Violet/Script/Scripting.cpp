#include "stdafx.h"
#include "Scripting.hpp"

#include "Utils/Profiling.hpp"

#include "EigenLib.hpp"
#include "Utils.hpp"


#include <iostream>

#define GAMELIBNAME "game"

//use a subset of lua's standard libs
static const luaL_Reg loadedlibs[] = {
	{ "_G", luaopen_base },
	{ LUA_LOADLIBNAME, luaopen_package },
	{ LUA_TABLIBNAME, luaopen_table },
	{ LUA_MATHLIBNAME, luaopen_math },
	{ EIGENLIBNAME, open_eigen_lib },
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
		lua_pop(L, 1);  // remove lib
	}
    
    luaU_push_mem_fn<Scripting, &Scripting::OpenLib>(L, this);
    luaU_require(L, GAMELIBNAME, 1);
    lua_pop(L, 1);  // remove lib
    
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

std::string Scripting::RunStr(const std::string& code)
{
    if (luaL_dostring(L, code.c_str()) != LUA_OK)
    {
        const char* msg = lua_tostring(L, -1);
        lua_pop(L, 1);
        throw std::runtime_error(msg);
    }
    
    std::string ret;
    while (lua_gettop(L))
    {
        lua_getglobal(L, "tostring");
        lua_pushvalue(L, 1);
        lua_call(L, 1, 1);
        const char* text = lua_tostring(L, -1);
        lua_pop(L, 1); //pop the string
        lua_remove(L, 1); //remove the result
        
        if (ret.size()) ret += ", ";
        ret += text;
    }
    
    return ret;
}
