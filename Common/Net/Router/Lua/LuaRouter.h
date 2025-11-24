//
// Created by 64658 on 2025/4/10.
//

#ifndef APP_LUAROUTER_H
#define APP_LUAROUTER_H
#include "Lua/Engine/Define.h"
namespace acs
{
	namespace LuaRouter
	{
		extern int Send(lua_State * L);
		extern int Call(lua_State * L);
		extern int Broadcast(lua_State * L);
	};
}


#endif //APP_LUAROUTER_H
