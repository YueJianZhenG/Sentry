//
// Created by yjz on 2022/11/22.
//
#include "LuaModule.h"
#include "Event/Base/IEvent.h"
#include "Lua/Engine/Function.h"
#include "Util/Tools/TimeHelper.h"
#include "Log/Common/CommonLogDef.h"


namespace Lua
{
	LuaModule::LuaModule(lua_State* lua, std::string name, int ref)
			: mLua(lua), mName(std::move(name)), mRef(ref)
	{
		this->mTimerID.fill(0);
		this->InitModule();
	}

	LuaModule::~LuaModule()
	{
		if(this->mRef != 0)
		{
			luaL_unref(this->mLua, LUA_REGISTRYINDEX, this->mRef);
		}
	}

	void LuaModule::OnHotfix()
	{
		this->mCaches.clear();
		lua_pushnil(this->mLua);
		while (lua_next(this->mLua, -2) != 0)
		{
			if (lua_isfunction(this->mLua, -1))
			{
				size_t size = 0;
				const char * func = lua_tolstring(this->mLua, -2, &size);
				{
					this->mCaches.emplace_back(func, size);
				}
			}
			lua_pop(this->mLua, 1);
		}
		this->InitEvent();
	}

	void LuaModule::InitModule()
	{
		this->SetMember("__name", this->mName);
		this->SetMember("__time", help::Time::NowSec());
		lua_rawgeti(this->mLua, LUA_REGISTRYINDEX, this->mRef);

		this->mCaches.clear();
		lua_pushnil(this->mLua);
		while (lua_next(this->mLua, -2) != 0)
		{
			if (lua_isfunction(this->mLua, -1))
			{
				size_t size = 0;
				const char * func = lua_tolstring(this->mLua, -2, &size);
				this->mCaches.emplace_back(func, size);
			}
			lua_pop(this->mLua, 1);
		}
		this->InitEvent();
	}

	void LuaModule::InitEvent()
	{
		if(this->HasFunction("OnUpdate"))
		{
			long long & timerId = this->mTimerID[0];
			help::OnOneSecondEvent::Remove(timerId);
			timerId = help::OnOneSecondEvent::Add([this](int tick)
			{
				this->Call("OnUpdate", tick);
			});
		}

		if(this->HasFunction("OnNewDay"))
		{
			long long & timerId = this->mTimerID[1];
			help::OnOneSecondEvent::Remove(timerId);
			timerId = help::OnNewDayEvent::Add([this](int day, int week)
			{
				this->Call("OnNewDay", day, week);
			});
		}

		if(this->HasFunction("OnNewHour"))
		{
			long long & timerId = this->mTimerID[2];
			help::OnOneSecondEvent::Remove(timerId);
			timerId = help::OnNewHourEvent::Add([this](int day, int hour)
			{
				this->Call("OnNewHour", day, hour);
			});
		}

		if(this->HasFunction("OnNewMinute"))
		{
			long long & timerId = this->mTimerID[3];
			help::OnOneSecondEvent::Remove(timerId);
			timerId = help::OnNewMinuteEvent::Add([this](int hour, int minute)
			{
				this->Call("OnNewMinute", hour, minute);
			});
		}
	}

	bool LuaModule::HasFunction(const std::string & name)
	{
		return std::any_of(this->mCaches.begin(), this->mCaches.end(),
				[&name](const std::string& key)
				{
					return name == key;
				});
	}

	bool LuaModule::GetFunction(const std::string& name)
	{
		if(!this->HasFunction(name))
		{
			return false;
		}
		lua_settop(this->mLua, 0);
		lua_rawgeti(this->mLua, LUA_REGISTRYINDEX, this->mRef);
		if (!lua_istable(this->mLua, -1))
		{
			return false;
		}
		lua_getfield(this->mLua, -1, name.c_str());

		if (!lua_isfunction(this->mLua, -1))
		{
			return false;
		}
		lua_pushvalue(this->mLua, -2);
		return true;
	}

	bool LuaModule::GetMetaFunction(const std::string& name) noexcept
	{
		lua_rawgeti(this->mLua, LUA_REGISTRYINDEX, this->mRef);
		if(!lua_istable(this->mLua, -1))
		{
			return false;
		}
		luaL_getmetafield(this->mLua, -1, name.c_str());
		if (!lua_isfunction(this->mLua, -1))
		{
			return false;
		}
		lua_pushvalue(this->mLua, -2);
		return true;
	}

	void LuaModule::SplitError(std::string& error)
	{
		size_t count = 0;
		const char * str = luaL_tolstring(this->mLua, -1, &count);
		do
		{
			if(str == nullptr)
			{
				break;
			}
			const char * errorInfo = strrchr(str, '/');
			if(errorInfo == nullptr)
			{
				errorInfo = strrchr(str, '\\');
			}
			if(errorInfo != nullptr)
			{
				error.assign(errorInfo);
				break;
			}
			error.assign(str, count);
		}
		while(false);
		lua_pop(this->mLua, 1);
	}

	void LuaModule::OnCallError(const std::string & func)
	{
		std::string error;
		this->SplitError(error);
		std::unique_ptr<custom::LogInfo> logInfo = std::make_unique<custom::LogInfo>();
		{
			logInfo->Level = custom::LogLevel::Error;
			logInfo->Content = fmt::format("[{}.{}] {}", this->mName, func, error);
		}
		Debug::Log(logInfo);
	}

}