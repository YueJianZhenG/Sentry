//
// Created by yjz on 2022/4/5.
//

#ifndef _SERVICE_H_
#define _SERVICE_H_
#include"Script/LuaInclude.h"
#include"Component/RpcService/ServiceComponent.h"
namespace Lua
{
	namespace Service
	{
		int Call(lua_State * lua);
		int AllotAddress(lua_State * lua);
	}
}

#endif //_SERVICE_H_
