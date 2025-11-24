//
// Created by 64658 on 2025/9/1.
//

#ifndef APP_LUAXML_H
#define APP_LUAXML_H

#include "Lua/Engine/LuaInclude.h"
namespace lua
{
	namespace xml
	{
		int encode(lua_State * L);
		int decode(lua_State * L);
		int write(lua_State * L, const char * str, size_t size);
	}
}

#endif //APP_LUAXML_H
