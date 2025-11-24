#pragma once
#include <vector>
#include "Lua/Engine/Function.h"
namespace Lua
{
	class Table
	{
	 public:
		Table(lua_State* luaEnv, int ref);

		~Table();

	 public:
		static bool Get(lua_State * lua, const std::string & name);
		static std::unique_ptr<Table> Create(lua_State* luaEnv, const std::string& name);

	 public:
		inline int Length();

		inline bool Get(const char * field, std::string & value)
		{
			lua_rawgeti(this->mLuaEnv, LUA_REGISTRYINDEX, this->ref);
			if (!lua_istable(this->mLuaEnv, -1))
			{
				lua_pop(this->mLuaEnv, 1);
				return false;
			}
			lua_getfield(this->mLuaEnv, -1, field);
			if(!lua_isstring(this->mLuaEnv, -1))
			{
				lua_pop(this->mLuaEnv, 2);
				return false;
			}
			size_t count = 0;
			const char * str = luaL_tolstring(this->mLuaEnv, -1, &count);
			value.assign(str, count);
			lua_pop(this->mLuaEnv, 2);
			return true;
		}

		inline bool Get(const char * field, std::vector<std::string> & value)
		{
			lua_rawgeti(this->mLuaEnv, LUA_REGISTRYINDEX, this->ref);
			if (!lua_istable(this->mLuaEnv, -1))
			{
				lua_pop(this->mLuaEnv, 1);
				return false;
			}

			lua_getfield(this->mLuaEnv, -1, field);
			if (!lua_istable(this->mLuaEnv, -1))
			{
				lua_pop(this->mLuaEnv, 2);
				return false;
			}

			lua_pushnil(this->mLuaEnv);
			while (lua_next(this->mLuaEnv, -2) != 0)
			{
				if (!lua_isstring(this->mLuaEnv, -1))
				{
					lua_pop(this->mLuaEnv, 3);
					return false;
				}

				size_t len = 0;
				const char* str = lua_tolstring(this->mLuaEnv, -1, &len);
				value.emplace_back(str, len);

				lua_pop(this->mLuaEnv, 1);
			}

			lua_pop(this->mLuaEnv, 2);
			return true;
		}

		template<typename T>
		inline std::enable_if_t<std::is_integral<T>::value, bool> Get(const char * field, std::vector<T> & value)
		{
			lua_rawgeti(this->mLuaEnv, LUA_REGISTRYINDEX, this->ref);
			if (!lua_istable(this->mLuaEnv, -1))
			{
				lua_pop(this->mLuaEnv, 1);
				return false;
			}

			lua_getfield(this->mLuaEnv, -1, field);
			if (!lua_istable(this->mLuaEnv, -1))
			{
				lua_pop(this->mLuaEnv, 2);
				return false;
			}

			lua_pushnil(this->mLuaEnv);
			while (lua_next(this->mLuaEnv, -2) != 0)
			{
				if (!lua_isinteger(this->mLuaEnv, -1))
				{
					lua_pop(this->mLuaEnv, 3);
					return false;
				}
				T number = (T)lua_tointeger(this->mLuaEnv, -1);
				value.emplace_back(number);
				lua_pop(this->mLuaEnv, 1);
			}
			lua_pop(this->mLuaEnv, 2);
			return true;
		}

		template<typename T>
		inline std::enable_if_t<std::is_floating_point<T>::value, bool> Get(const char * field, std::vector<T> & value)
		{
			lua_rawgeti(this->mLuaEnv, LUA_REGISTRYINDEX, this->ref);
			if (!lua_istable(this->mLuaEnv, -1))
			{
				lua_pop(this->mLuaEnv, 1);
				return false;
			}

			lua_getfield(this->mLuaEnv, -1, field);
			if (!lua_istable(this->mLuaEnv, -1))
			{
				lua_pop(this->mLuaEnv, 2);
				return false;
			}

			lua_pushnil(this->mLuaEnv);
			while (lua_next(this->mLuaEnv, -2) != 0)
			{
				if (!lua_isnumber(this->mLuaEnv, -1))
				{
					lua_pop(this->mLuaEnv, 3);
					return false;
				}
				T number = (T)lua_tonumber(this->mLuaEnv, -1);
				value.emplace_back(number);
				lua_pop(this->mLuaEnv, 1);
			}
			lua_pop(this->mLuaEnv, 2);
			return true;
		}


		template<typename T>
		inline std::enable_if_t<std::is_integral<T>::value, bool> Get(const char * field, T & value)
		{
			lua_rawgeti(this->mLuaEnv, LUA_REGISTRYINDEX, this->ref);
			if (!lua_istable(this->mLuaEnv, -1))
			{
				lua_pop(this->mLuaEnv, 1);
				return false;
			}
			lua_getfield(this->mLuaEnv, -1, field);
			if (!lua_isinteger(this->mLuaEnv, -1))
			{
				lua_pop(this->mLuaEnv, 2);
				return false;
			}

			value = static_cast<T>(lua_tointeger(this->mLuaEnv, -1));
			lua_pop(this->mLuaEnv, 2);
			return true;
		}

		template<typename T>
		inline std::enable_if_t<std::is_floating_point<T>::value, bool> Get(const char * field, T & value)
		{
			lua_rawgeti(this->mLuaEnv, LUA_REGISTRYINDEX, this->ref);
			if (!lua_istable(this->mLuaEnv, -1))
			{
				lua_pop(this->mLuaEnv, 1);
				return false;
			}
			lua_getfield(this->mLuaEnv, -1, field);
			if (!lua_isnumber(this->mLuaEnv, -1))
			{
				lua_pop(this->mLuaEnv, 2);
				return false;
			}
			value = static_cast<T>(lua_tonumber(this->mLuaEnv, -1));
			lua_pop(this->mLuaEnv, 2);
			return true;
		}

	 public:
		std::unique_ptr<Table> GetTable(int index);
		std::unique_ptr<Table> GetTable(const std::string& name);
		std::unique_ptr<Function> GetFunction(const std::string& name);

	 private:
		int ref;
		lua_State* mLuaEnv;
	};

	int Table::Length()
	{
		lua_rawgeti(this->mLuaEnv, LUA_REGISTRYINDEX, this->ref);
		if (!lua_istable(this->mLuaEnv, -1))
		{
			return 0;
		}
		lua_len(this->mLuaEnv, -1);
		return lua_tointeger(this->mLuaEnv, -1);
	}
}
