//
// Created by yjz on 2022/4/4.
//

#include"Timer.h"
#include"Entity/Actor/App.h"
#include"Timer/Timer/DelayTimer.h"
#include"Timer/Component/TimerComponent.h"
using namespace acs;

namespace lua
{
	std::unique_ptr<LuaTimer> MakeTimer(lua_State * lua, int index, int ms)
	{
		int cor = 0;
		std::unique_ptr<LuaTimer> luaTimer;
		if(lua_isfunction(lua, index))
		{
			lua_pushvalue(lua, index);
			int ref = luaL_ref(lua, LUA_REGISTRYINDEX);
			if(lua_pushthread(lua) == 0)
			{
				cor = luaL_ref(lua, LUA_REGISTRYINDEX);
			}
			long long id = TimerComponent::MakeTimerID();
			luaTimer = std::make_unique<LuaTimer>(id, lua, ref, ms, cor);
		}
		else if(lua_istable(lua, index))
		{
			if(!lua_isstring(lua, index + 1))
			{
				return nullptr;
			}
			std::string method(lua_tostring(lua, index + 1));

			lua_pushvalue(lua, index);
			int ref = luaL_ref(lua, LUA_REGISTRYINDEX);
			if(lua_pushthread(lua) == 0)
			{
				cor = luaL_ref(lua, LUA_REGISTRYINDEX);
			}
			long long id = TimerComponent::MakeTimerID();
			luaTimer = std::make_unique<LuaTimer>(id, lua, ref, method, ms, cor);
		}
		return luaTimer;
	}

	int Timer::Add(lua_State* lua)
    {
		static TimerComponent* timerComponent = App::Get<TimerComponent>();
		{
			int index = 1;
			int timeout = 0;
			if(lua_isinteger(lua, 1))
			{
				index++;
				timeout = (int)luaL_checkinteger(lua, 1);
			}
			std::unique_ptr<LuaTimer> luaTimer = MakeTimer(lua, index, timeout);
			if(luaTimer == nullptr)
			{
				LOG_ERROR("add timer error");
				return 0;
			}
			long long timerId = luaTimer->GetTimerId();
			timerComponent->AddTimer(std::move(luaTimer));
			lua_pushinteger(lua, timerId);
			return 1;
		}
    }

	int Timer::Remove(lua_State* lua)
	{
		static TimerComponent* timerComponent = App::Get<TimerComponent>();

		long long timerId = luaL_checkinteger(lua, 1);
        lua_pushboolean(lua, timerComponent->CancelTimer(timerId));
		return 1;
	}
}