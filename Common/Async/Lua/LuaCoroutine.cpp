#include"LuaCoroutine.h"
#include"Entity/Actor/App.h"
#include"Log/Lua/LuaLogger.h"
#include"Async/Lua/LuaWaitTaskSource.h"
#include"Timer/Component/TimerComponent.h"

using namespace acs;

namespace Lua
{
	bool Coroutine::IsRunning(lua_State* lua)
	{
		if (lua == nullptr)
		{
			return false;
		}
		int top = lua_gettop(lua);
		return top == 1 && lua_isthread(lua, 1);
	}

	int Coroutine::Sleep(lua_State* lua)
	{
		unsigned int ms = (unsigned int)luaL_checkinteger(lua, 1);
		static TimerComponent* timerComponent = nullptr;
		if (timerComponent == nullptr)
		{
			timerComponent = App::Get<TimerComponent>();
			if (timerComponent == nullptr)
			{
				luaL_error(lua, "not find TimerComponent");
				return 0;
			}
		}

		lua_pushthread(lua);
		LuaWaitTaskSource* luaRpcTaskSource = new LuaWaitTaskSource(lua);
		timerComponent->Timeout(ms, [luaRpcTaskSource]()
		{
			luaRpcTaskSource->SetResult();
			delete luaRpcTaskSource;
		});
		return luaRpcTaskSource->Await();
	}

	void Coroutine::Resume(lua_State* cor, int args)
	{
		int ret = 0;
		int code = lua_status(cor);
		switch (lua_status(cor))
		{
			case LUA_OK:
			LOG_ERROR("Coroutine already finished.");
				break;
			case LUA_YIELD:
			{
				code = lua_resume(cor, nullptr, args, &ret);
				if (code != LUA_OK && code != LUA_YIELD)
				{
					const char* errorMsg = lua_tostring(cor, -1);
					std::unique_ptr<custom::LogInfo> logInfo = std::make_unique<custom::LogInfo>();
					{
						logInfo->Level = custom::LogLevel::Error;
						logInfo->Content = fmt::format("Error during coroutine resume: {}", errorMsg);
						Debug::Log(logInfo);
					}
					lua_pop(cor, 1);
				}
				if (ret > 0)
				{
					lua_pop(cor, ret);
				}
				break;
			}
			default:
			LOG_ERROR("coroutine status:{}", code);
				break;
		}
	}
}
