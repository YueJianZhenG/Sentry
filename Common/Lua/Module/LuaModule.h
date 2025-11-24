//
// Created by yjz on 2022/11/22.
//

#pragma once
#include "XCode/XCode.h"
#include "Lua/Engine/Function.h"
namespace Lua
{
	class LuaModule
	{
	public:
		LuaModule(lua_State* lua, std::string name, int ref);
		~LuaModule();
	public:
		void OnHotfix();
		const std::string & Name() const { return this->mName; }
	public:
		template<typename ... Args>
		int Call(const std::string & func, Args && ... args);
		template<typename ... Args>
		int Await(const std::string & func, Args && ... args);
	public:
		bool GetFunction(const std::string& name);
		bool HasFunction(const std::string & name);
		bool GetMetaFunction(const std::string & name) noexcept;
	public:
		void SplitError(std::string & error);
		template<typename T>
		inline void SetMember(const char* key, const T & value)
		{
			lua_rawgeti(this->mLua, LUA_REGISTRYINDEX, this->mRef);
			{
				Lua::Parameter::Write(this->mLua, value);
				lua_setfield(this->mLua, -2, key);
			}
		}
		inline lua_State * GetLuaEnv() { return this->mLua;}
	private:
		void InitEvent();
		void InitModule();
		void OnCallError(const std::string & func);
	private:
		int mRef;
		lua_State* mLua;
		const std::string mName;
		std::vector<std::string> mCaches;
		std::array<long long, 4> mTimerID;
	};

	template<typename... Args>
	int LuaModule::Call(const std::string& func, Args&& ... args)
	{
		if(!this->GetFunction(func))
		{
			return XCode::CallFunctionNotExist;
		}
		Parameter::WriteArgs<Args...>(this->mLua, std::forward<Args>(args)...);
		if (lua_pcall(this->mLua, sizeof...(Args) + 1, 0, 0) != LUA_OK)
		{
			this->OnCallError(func);
			return XCode::CallLuaFunctionFail;
		}
		return XCode::Ok;
	}

	template<typename... Args>
	int LuaModule::Await(const std::string& func, Args&& ... args)
	{
		lua_settop(this->mLua, 0);
		if(!this->HasFunction(func))
		{
			return XCode::CallFunctionNotExist;
		}
		this->GetMetaFunction("Await");
		lua_pushstring(this->mLua, func.c_str());
		std::unique_ptr<WaitLuaTaskSource> task = std::make_unique<WaitLuaTaskSource>();
		{
			Lua::Parameter::Write<WaitLuaTaskSource*>(this->mLua, task.get());
			Lua::Parameter::WriteArgs(this->mLua, std::forward<Args>(args)...);
		}
		if(lua_pcall(this->mLua, sizeof...(Args) + 3, 1, 0) != LUA_OK)
		{
			this->OnCallError(func);
			return XCode::CallLuaFunctionFail;
		}
		task->Await<void>();
		return XCode::Ok;
	}
}
