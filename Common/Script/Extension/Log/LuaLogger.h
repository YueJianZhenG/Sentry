#pragma once

#include "Script/LuaInclude.h"

namespace Lua
{
	namespace Log
	{
		extern int DebugLog(lua_State* luaEnv);

		extern int DebugInfo(lua_State* luaEnv);

		extern int DebugError(lua_State* luaEnv);

		extern int DebugWarning(lua_State* luaEnv);

		extern void GetLuaString(lua_State* luaEnv, std::string & ret);
	}
}