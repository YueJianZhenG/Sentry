//
// Created by yjz on 2022/4/4.
//

#include"Timer.h"
#include"Entity/App/App.h"
#include<memory>
#include"Timer/Timer/DelayTimer.h"
#include"Timer/Component/TimerComponent.h"
using namespace Tendo;

namespace Lua
{
	int Timer::Add(lua_State* lua)
    {
        TimerComponent* timerComponent = App::Inst()->GetTimerComponent();
        if (timerComponent == nullptr)
        {
            luaL_error(lua, "TimerComponent Is Null");
            return 0;
        }
        unsigned int ms = (unsigned int)luaL_checkinteger(lua, 1);
        lua_pushvalue(lua, 2);
        if (!lua_isfunction(lua, -1))
        {
            luaL_error(lua, "add timer type = [%s]", luaL_typename(lua, -1));
            return 0;
        }
        int ref = luaL_ref(lua, LUA_REGISTRYINDEX);
        std::shared_ptr<LuaTimer> luaTimer(new LuaTimer(ms, ref, lua));
        unsigned int timerId = timerComponent->AddTimer(luaTimer);
        lua_pushinteger(lua, timerId);
        return 1;
    }

	int Timer::Remove(lua_State* lua)
	{
        TimerComponent* timerComponent = App::Inst()->GetTimerComponent();
        if (timerComponent == nullptr)
        {
            luaL_error(lua, "TimerComponent Is Null");
            return 0;
        }
        unsigned int timerId = (unsigned int) luaL_checkinteger(lua, 1);//Lua::Parameter::Read<unsigned int>(lua, 2);
        lua_pushboolean(lua, timerComponent->CancelTimer(timerId));
		return 1;
	}
}