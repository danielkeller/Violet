#include "stdafx.h"
#include "Utils.hpp"

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
    lua_pop(L, 1); /* remove module loader */
}

std::string luaU_checkstdstring(lua_State* L, int arg)
{
	size_t len;
	const char* str = luaL_checklstring(L, arg, &len);
	return{ str, str + len };
}
