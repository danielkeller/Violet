#include "stdafx.h"
#include "EigenLib.hpp"
#include "Utils.hpp"

#include <sstream>

//consider overriding eigen's assert to throw, so we can report error locations and such

//eigen makes this distinction so we must as well
using LuaVector = Eigen::Matrix<lua_Number, Eigen::Dynamic, 1>;
using LuaMatrix = Eigen::Matrix<lua_Number, Eigen::Dynamic, Eigen::Dynamic>;

static const char* LUAVECTOR_TNAME = "eigen.vector";

//returns the argument as vector, or throws an error
LuaVector& tovector(lua_State *L, int arg)
{
	void* v = luaL_checkudata(L, arg, LUAVECTOR_TNAME);
	return *reinterpret_cast<LuaVector*>(v);
}

static int vector_tostring(lua_State *L)
{
	std::stringstream ss;
	ss << tovector(L, 1).transpose();
	lua_pop(L, 1);
	lua_pushlstring(L, ss.str().c_str(), ss.str().size());
	return 1;
}

static int vector_ctor(lua_State *L)
{
	int nargs = lua_gettop(L);
	LuaVector::Index dim = 0;
	if (nargs > 0) dim = luaL_checkinteger(L, 1);
	luaL_getmetatable(L, LUAVECTOR_TNAME);
	luaU_new<LuaVector>(L, dim);
	return 1;
}

static int vector_commainit(lua_State *L)
{
	int nargs = lua_gettop(L);
	auto& vec = tovector(L, 1);
	if (nargs == 1) return 1; //return self

	auto init = //there has to be a better way
		lua_isnumber(L, 2) ? vec << luaL_checknumber(L, 2)
		: luaL_testudata(L, 2, LUAVECTOR_TNAME) ? vec << tovector(L, 2)
		: vec << luaL_argerror(L, 1, "expected number or vector");

	for (int arg = 3; arg <= nargs; ++arg)
	{
		if (lua_isnumber(L, arg))
			init, luaL_checknumber(L, arg);
		else if (luaL_testudata(L, arg, LUAVECTOR_TNAME))
			init, tovector(L, arg);
		else
			luaL_argerror(L, 1, "expected number or vector");
	}

	lua_pop(L, nargs-1);
	return 1;
}

static const luaL_Reg vectorMetaMethods[] =
	{ { "__gc", luaU_delete<LuaVector> }
	, { "__tostring", vector_tostring }
	, { "assign", vector_commainit }
	, { NULL, NULL }
};

static const luaL_Reg eigenlib[] =
	{ { "vector", vector_ctor }
	, { NULL, NULL }
};

int open_eigen_lib(lua_State* L)
{
	if (luaL_newmetatable(L, LUAVECTOR_TNAME))
		luaL_setfuncs(L, vectorMetaMethods, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	luaL_newlib(L, eigenlib);
	return 1;
}
