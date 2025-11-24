//
// Created by 64658 on 2025/4/10.
//

#include "LuaRouter.h"
#include "XCode/XCode.h"
#include "Entity/Actor/App.h"
#include "Router/Component/RouterComponent.h"
namespace acs
{
	int LuaRouter::Send(lua_State* L)
	{
		size_t count = 0;
		int code = XCode::Ok;
		static std::string func;
		int id = (int)luaL_checkinteger(L, 1);
		const char * str = luaL_checklstring(L, 2, &count);
		static RouterComponent * routerComponent = App::Get<RouterComponent>();
		do
		{
			func.assign(str, count);
			std::unique_ptr<rpc::Message> message;
			code = acs::App::Inst()->MakeMessage(L, 3, func, message);
			if(code != XCode::Ok)
			{
				break;
			}
			message->SetSockId(id);
			code = routerComponent->Send(id, message);
		}
		while(false);
		lua_pushinteger(L, code);
		return 1;
	}

	int LuaRouter::Call(lua_State* L)
	{
		size_t count = 0;
		int code = XCode::Ok;
		static std::string func;
		int id = (int)luaL_checkinteger(L, 1);
		const char * str = luaL_checklstring(L, 2, &count);
		static RouterComponent * routerComponent = App::Get<RouterComponent>();
		do
		{
			func.assign(str, count);
			std::unique_ptr<rpc::Message> message;
			code = acs::App::Inst()->MakeMessage(L, 3, func, message);
			if(code != XCode::Ok)
			{
				break;
			}
			lua_pushthread(L);
			message->SetSockId(id);
			return routerComponent->LuaCall(L, id, message);
		}
		while(false);
		lua_pushinteger(L, code);
		return 1;
	}

	int LuaRouter::Broadcast(lua_State* L)
	{
		size_t count = 0;
		int code = XCode::Ok;
		static std::string func;
		const char * str = luaL_checklstring(L, 1, &count);
		static RouterComponent * routerComponent = App::Get<RouterComponent>();
		do
		{
			func.assign(str, count);
			std::unique_ptr<rpc::Message> message;
			code = acs::App::Inst()->MakeMessage(L, 2, func, message);
			if(code != XCode::Ok)
			{
				break;
			}
			lua_pushthread(L);
			routerComponent->Broadcast(message);
		}
		while(false);
		lua_pushinteger(L, code);
		return 1;
	}
}