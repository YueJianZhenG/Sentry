//
// Created by yjz on 2022/4/4.
//

#ifndef _TIMER_H_
#define _TIMER_H_
#include"Lua/LuaInclude.h"
namespace Lua
{
	namespace Timer
	{
		extern int AddTimer(lua_State * lua);
		extern int CancelTimer(lua_State * lua);
	}
}

#endif //_TIMER_H_