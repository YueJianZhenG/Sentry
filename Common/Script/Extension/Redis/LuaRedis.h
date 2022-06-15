//
// Created by yjz on 2022/6/15.
//

#ifndef _LUAREDIS_H_
#define _LUAREDIS_H_
#include"Script/LuaInclude.h"
namespace Sentry
{
	class RedisRequest;
	class RedisClientContext;
	class MainRedisComponent;
}

using namespace Sentry;
namespace Lua
{
	namespace Redis
	{
		int Run(lua_State * lua);
		int Call(lua_State * lua);
		int Send(lua_State * lua, MainRedisComponent * redisComponent,
			std::shared_ptr<RedisClientContext> redisClientContext, std::shared_ptr<RedisRequest> request);
	};
}

#endif //_LUAREDIS_H_
