#pragma once

#include"Lua/Engine/Function.h"
namespace Lua
{
	class Table
	{
	 public:
		Table(lua_State* luaEnv, int ref, const std::string& name);

		~Table();

	 public:
		static bool Get(lua_State * lua, const std::string & name);
		static std::unique_ptr<Table> Create(lua_State* luaEnv, const std::string& name);

	 public:
		template<typename T>
		T GetMember(const char* name);

	 public:
		int GetRef() const
		{
			return this->ref;
		}
		std::string ToJson();
		std::unique_ptr<Table> GetTable(const std::string& name);
		std::unique_ptr<Function> GetFunction(const std::string& name);

		bool Serialization(std::string& outString);

	 private:
		int ref;
		lua_State* mLuaEnv;
		std::string mTableName;
	};

	template<typename T>
	T Table::GetMember(const char* name)
	{
		lua_rawgeti(this->mLuaEnv, LUA_REGISTRYINDEX, this->ref);
		if (lua_istable(this->mLuaEnv, -1))
		{
			lua_getfield(this->mLuaEnv, -1, name);
			return Parameter::Read<T>(this->mLuaEnv, -1);
		}
		return T();
	}
}
