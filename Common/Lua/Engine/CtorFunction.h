#pragma once

#include "LuaInclude.h"
#include "Lua/Engine/ClassNameProxy.h"

namespace Lua
{
	namespace CtorFunction
	{
		template<typename T, typename... Args>
		int CreateUserData(lua_State* luaEnv, Args... args)
		{
			void* userdata = lua_newuserdata(luaEnv, sizeof(PtrProxy<T>));
			new(userdata) PtrProxy<T>(new T(std::forward<Args>(args)...), true);
			const std::string & name = lua::ClassFactory<T>::name;
			lua_getglobal(luaEnv, name.c_str());
			lua_setmetatable(luaEnv, -2);
			return 1;
		}
	}// namespace CtorFunction
	namespace CtorFunction
	{
		template<typename T>
		static int PushCtor(lua_State* luaEnv)
		{
			void* userdata = lua_newuserdata(luaEnv, sizeof(PtrProxy<T>));
			new(userdata) PtrProxy<T>(new T(), true);
			const std::string & name = lua::ClassFactory<T>::name;
			lua_getglobal(luaEnv, name.c_str());
			lua_setmetatable(luaEnv, -2);
			return 1;
		}

		template<typename T, typename T1>
		static int PushCtor(lua_State* luaEnv)
		{
			T1 t1 = Parameter::Read<T1>(luaEnv, 1);
			return CreateUserData<T>(luaEnv, t1);
		}

		template<typename T, typename T1, typename T2>
		static int PushCtor(lua_State* luaEnv)
		{
			T1 t1 = Parameter::Read<T1>(luaEnv, 1);
			T2 t2 = Parameter::Read<T2>(luaEnv, 2);
			return CreateUserData<T>(luaEnv, t1, t2);
		}

		template<typename T, typename T1, typename T2, typename T3>
		static int PushCtor(lua_State* luaEnv)
		{
			T1 t1 = Parameter::Read<T1>(luaEnv, 1);
			T2 t2 = Parameter::Read<T2>(luaEnv, 2);
			T3 t3 = Parameter::Read<T3>(luaEnv, 3);
			return CreateUserData<T>(luaEnv, t1, t2, t3);
		}

		template<typename T, typename T1, typename T2, typename T3, typename T4>
		static int PushCtor(lua_State* luaEnv)
		{
			T1 t1 = Parameter::Read<T1>(luaEnv, 1);
			T2 t2 = Parameter::Read<T2>(luaEnv, 2);
			T3 t3 = Parameter::Read<T3>(luaEnv, 3);
			T4 t4 = Parameter::Read<T4>(luaEnv, 4);
			return CreateUserData<T>(luaEnv, t1, t2, t3, t4);
		}

		template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
		static int PushCtor(lua_State* luaEnv)
		{
			T1 t1 = Parameter::Read<T1>(luaEnv, 1);
			T2 t2 = Parameter::Read<T2>(luaEnv, 2);
			T3 t3 = Parameter::Read<T3>(luaEnv, 3);
			T4 t4 = Parameter::Read<T4>(luaEnv, 4);
			T5 t5 = Parameter::Read<T5>(luaEnv, 5);
			return CreateUserData<T>(luaEnv, t1, t2, t3, t4, t5);
		}

		template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
		static int PushCtor(lua_State* luaEnv)
		{
			T1 t1 = Parameter::Read<T1>(luaEnv, 1);
			T2 t2 = Parameter::Read<T2>(luaEnv, 2);
			T3 t3 = Parameter::Read<T3>(luaEnv, 3);
			T4 t4 = Parameter::Read<T4>(luaEnv, 4);
			T5 t5 = Parameter::Read<T5>(luaEnv, 5);
			T6 t6 = Parameter::Read<T6>(luaEnv, 6);
			return CreateUserData<T>(luaEnv, t1, t2, t3, t4, t5, t6);
		}

		template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
		static int PushCtor(lua_State* luaEnv)
		{
			T1 t1 = Parameter::Read<T1>(luaEnv, 1);
			T2 t2 = Parameter::Read<T2>(luaEnv, 2);
			T3 t3 = Parameter::Read<T3>(luaEnv, 3);
			T4 t4 = Parameter::Read<T4>(luaEnv, 4);
			T5 t5 = Parameter::Read<T5>(luaEnv, 5);
			T6 t6 = Parameter::Read<T6>(luaEnv, 6);
			T7 t7 = Parameter::Read<T7>(luaEnv, 7);
			return CreateUserData<T>(luaEnv, t1, t2, t3, t4, t5, t6, t7);
		}

		template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
		static int PushCtor(lua_State* luaEnv)
		{
			T1 t1 = Parameter::Read<T1>(luaEnv, 1);
			T2 t2 = Parameter::Read<T2>(luaEnv, 2);
			T3 t3 = Parameter::Read<T3>(luaEnv, 3);
			T4 t4 = Parameter::Read<T4>(luaEnv, 4);
			T5 t5 = Parameter::Read<T5>(luaEnv, 5);
			T6 t6 = Parameter::Read<T6>(luaEnv, 6);
			T7 t7 = Parameter::Read<T7>(luaEnv, 7);
			T8 t8 = Parameter::Read<T8>(luaEnv, 8);
			return CreateUserData<T>(luaEnv, t1, t2, t3, t4, t5, t6, t7, t8);
		}

		template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
		static int PushCtor(lua_State* luaEnv)
		{
			T1 t1 = Parameter::Read<T1>(luaEnv, 1);
			T2 t2 = Parameter::Read<T2>(luaEnv, 2);
			T3 t3 = Parameter::Read<T3>(luaEnv, 3);
			T4 t4 = Parameter::Read<T4>(luaEnv, 4);
			T5 t5 = Parameter::Read<T5>(luaEnv, 5);
			T6 t6 = Parameter::Read<T6>(luaEnv, 6);
			T7 t7 = Parameter::Read<T7>(luaEnv, 7);
			T8 t8 = Parameter::Read<T8>(luaEnv, 8);
			T9 t9 = Parameter::Read<T9>(luaEnv, 9);
			return CreateUserData<T>(luaEnv, t1, t2, t3, t4, t5, t6, t7, t8, t9);
		}
	}// namespace CtorFunction
}