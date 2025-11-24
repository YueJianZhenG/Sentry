//
// Created by yjz on 2022/4/4.
//

#ifndef APP_TIMER_H
#define APP_TIMER_H
#include"Lua/Engine/Define.h"
namespace lua
{
	namespace Timer
	{
		extern int Add(lua_State * lua);
		extern int Remove(lua_State * lua);
	}
}

#endif //APP_TIMER_H
