#pragma once

#include"Lua/Engine/Define.h"
namespace lua
{
	namespace Log
	{
		extern int Output(lua_State * lua);
		extern int Custom(lua_State * lua);
		extern void Error(lua_State * lua);
		extern int OnCallError(lua_State * lua);
	}

	namespace Console
	{
		extern int Show(lua_State* luaEnv);
	}
}