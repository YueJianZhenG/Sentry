#pragma once

#include "LuaParameter.h"
namespace Lua
{
	namespace FunctionParameter
	{
		template<typename T>
		struct FunctionStruct
		{
			static T Read(lua_State* lua, int index)
			{
				assert(false);
				return T();
			}

			static void Write(lua_State* lua, T data)
			{
				assert(false);
			}
		};

		template<typename T>
		inline T Read(lua_State* lua, int index)
		{
			return FunctionStruct<T>::Read(lua, index);
		}

		template<typename T>
		inline void Write(lua_State* lua, T data)
		{
		}
	}// namespace FunctionParameter

	namespace FunctionParameter
	{
		template<typename Ret, typename... Args>
		struct FunctionStruct<std::function<Ret(Args...)>>
		{
			static std::function<Ret(Args...)> Read(lua_State* lua, int index)
			{
				assert(lua_isfunction(lua, index));
				lua_pushvalue(lua, index);
				int ref = luaL_ref(lua, LUA_REGISTRYINDEX);
				std::function<Ret(Args...)> callBack = [lua, ref](Args... args)
				{
					lua_rawgeti(lua, LUA_REGISTRYINDEX, ref);
					Parameter::WriteArgs(lua, std::forward<Args>(args)...);
					if (lua_pcall(lua, sizeof...(Args), 1, 0) != 0)
					{
						printf("[lua error ] : %s", lua_tostring(lua, -1));
						lua_pop(lua, 1);
					}
					return Parameter::Read<Ret>(lua, -1);
				};
				return callBack;
			}
		};

		template<typename... Args>
		struct FunctionStruct<std::function<void(Args...)>>
		{
			static std::function<void(Args...)> Read(lua_State* lua, int index)
			{
				assert(lua_isfunction(lua, index));
				lua_pushvalue(lua, index);
				int ref = luaL_ref(lua, LUA_REGISTRYINDEX);
				std::function<void(Args...)> callBack = [lua, ref](Args... args)
				{
					lua_rawgeti(lua, LUA_REGISTRYINDEX, ref);
					Parameter::WriteArgs(lua, std::forward<Args>(args)...);
					if (lua_pcall(lua, sizeof...(Args), 0, 0) != 0)
					{
						printf("[lua error ] : %s", lua_tostring(lua, -1));
					}
				};
				return callBack;
			}
		};
	}// namespace FunctionParameter
}