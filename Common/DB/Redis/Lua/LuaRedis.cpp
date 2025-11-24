//
// Created by yjz on 2022/6/15.
//

#include "LuaRedis.h"
#include "Entity/Actor/App.h"
#include "Yyjson/Lua/ljson.h"
#include "Redis/Client/RedisDefine.h"
#include "Proto/Component/ProtoComponent.h"
#include "Redis/Component/RedisComponent.h"
#include "Redis/Component/RedisSubComponent.h"
using namespace acs;
namespace lua
{
	inline void ReadFromIndex(lua_State * lua, int index, ::redis::Request * request)
	{
		switch (lua_type(lua, index))
		{
			case LUA_TSTRING:
			{
				size_t size = 0;
				const char* str = lua_tolstring(lua, index, &size);
				request->AddParameter(str, size);
				break;
			}
			case LUA_TNUMBER:
			{
				if (lua_isinteger(lua, index))
				{
					long long num = lua_tointeger(lua, index);
					request->AddParameter(num);
					return;
				}
				double num = lua_tonumber(lua, index);
				request->AddParameter(std::to_string(num));
				break;
			}
			case LUA_TTABLE:
			{
				wrap::string<true> json;
				if(lua::yyjson::read(lua, index, json))
				{
					request->AddParameter(json.c_str(), json.size());
				}
				break;
			}
			case LUA_TBOOLEAN:
			{
				int val = lua_toboolean(lua, index);
				request->AddParameter(val);
				break;
			}
			default:
				luaL_error(lua, "type error : %s  %d", lua_typename(lua, index), lua_type(lua, index));
				return;
		}
	}

	int redis::Run(lua_State* lua)
    {
		static RedisComponent* redisComponent = App::Get<RedisComponent>();

		size_t count = 0;
		const char * cmd = luaL_checklstring(lua, 1, &count);
		if(cmd == nullptr || count <= 0)
		{
			return 0;
		}

        std::unique_ptr<::redis::Request> request = std::make_unique<::redis::Request>();
		{
			request->SetCommand(cmd, count);
			int count = (int)luaL_len(lua, 2);
			for (int i = 0; i < count; i++)
			{
				lua_geti(lua, 2, i + 1);
				int index = lua_absindex(lua, -1);
				ReadFromIndex(lua, index, request.get());
				lua_pop(lua, 1);
			}
		}
		int id = 0;
		lua_pushthread(lua);
		redisComponent->Send(request, id);
        return redisComponent->AddTask(new LuaRedisTask(lua, id))->Await();
    }

	int redis::Call(lua_State* lua)
    {
		static RedisComponent* redisComponent = App::Get<RedisComponent>();
		size_t count = 0;
		const char * name = luaL_checklstring(lua, 1, &count);

		RedisLuaData redisLuaData;
		redisLuaData.name.assign(name, count);
		switch(lua_type(lua, 2))
		{
			case LUA_TSTRING:
			{
				size_t size = 0;
				const char * str = lua_tolstring(lua, 2, &size);
				redisLuaData.json.append(str, size);
			}
				break;
			case LUA_TTABLE:
				lua::yyjson::read(lua, 2, redisLuaData.json);
				break;
			default:
				LOG_ERROR("parameter must table or string");
				return 0;
		}
		int rpcId = 0;
        lua_pushthread(lua);
		unsigned int timeout = 0;
		redisComponent->Send(redisLuaData, rpcId, timeout);
        return redisComponent->AddTask(new LuaRedisTask(lua, rpcId), timeout)->Await();
    }

    int redis::Send(lua_State *lua)
    {
		static RedisComponent* redisComponent = App::Get<RedisComponent>();

		size_t count = 0;
		const char * cmd = luaL_checklstring(lua, 1, &count);
		if(cmd == nullptr || count <= 0)
		{
			return 0;
		}

		std::unique_ptr<::redis::Request> request = std::make_unique<::redis::Request>();
		{
			request->SetCommand(cmd, count);
			int count = (int)luaL_len(lua, 2);
			for (int i = 0; i < count; i++)
			{
				lua_geti(lua, 2, i + 1);
				int index = lua_absindex(lua, -1);
				ReadFromIndex(lua, index, request.get());
				lua_pop(lua, 1);
			}
		}
		lua_pushboolean(lua, true);
		redisComponent->Send(request);
        return 1;
    }

	int sub_redis::Run(lua_State* L)
	{
		static RedisSubComponent* redisComponent = App::Get<RedisSubComponent>();

		size_t count = 0;
		const char * cmd = luaL_checklstring(L, 1, &count);

		std::unique_ptr<::redis::Request> request = std::make_unique<::redis::Request>();
		{
			request->SetCommand(cmd, count);
			int count = (int)luaL_len(L, 2);
			for (int i = 0; i < count; i++)
			{
				lua_geti(L, 2, i + 1);
				int index = lua_absindex(L, -1);
				ReadFromIndex(L, index, request.get());
				lua_pop(L, 1);
			}
		}
		int id = 0;
		lua_pushthread(L);
		redisComponent->Send(request, id);
		return redisComponent->AddTask(new LuaRedisTask(L, id))->Await();
	}

}