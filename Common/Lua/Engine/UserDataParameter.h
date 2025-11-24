#pragma once

#include "LuaInclude.h"
#include "ClassNameProxy.h"

namespace Lua
{
	namespace UserDataParameter
	{
		template<typename T>
		struct UserDataStruct
		{
			static T Read(lua_State* lua, int index)
			{
				return T();
			}

			static void Write(lua_State* lua, T data)
			{
				assert(false);
			}

			static void WritePtr(lua_State* lua, T data)
			{
				assert(false);
			}
		};

		template<typename T>
		struct UserDataStruct<T&>
		{
//			static T& Read(lua_State* lua, int index)
//			{
//				return T();
//			}

			static void Write(lua_State* lua, T& data)
			{
				size_t size = sizeof(Lua::PtrProxy<T>);
				new(lua_newuserdata(lua, size))PtrProxy<T>(&data);
				const std::string & name = lua::ClassFactory<T>::name;
				lua_getglobal(lua, name.c_str());
				if (lua_istable(lua, -1))
				{
					lua_setmetatable(lua, -2);
				}
			}
		};
	}
	namespace UserDataParameter
	{
		template<typename T>
		struct UserDataStruct<T*>
		{
			static T* Read(lua_State* lua, int index)
			{
				return PtrProxy<T>::Read(lua, index);
			}

			static void Write(lua_State* lua, T* data)
			{
				if (data == nullptr)
				{
					lua_pushnil(lua);
					return;
				}
				size_t size = sizeof(PtrProxy<T>);
				new(lua_newuserdata(lua, size))PtrProxy<T>(data);
				const std::string & typeName = lua::ClassFactory<T>::name;
				if (!typeName.empty())
				{
					lua_getglobal(lua, typeName.c_str());
					if (lua_istable(lua, -1))
					{
						lua_setmetatable(lua, -2);
					}
				}
			}

			static void WritePtr(lua_State* lua, T* data)
			{
				if (data == nullptr)
				{
					lua_pushnil(lua);
					return;
				}
				size_t size = sizeof(PtrProxy<T>);
				new(lua_newuserdata(lua, size))PtrProxy<T>(data, true);
				const std::string & typeName = lua::ClassFactory<T>::name;
				if (!typeName.empty())
				{
					lua_getglobal(lua, typeName.c_str());
					if (lua_istable(lua, -1))
					{
						lua_setmetatable(lua, -2);
					}
				}
			}
		};
	}
	namespace UserDataParameter
	{
		template<typename T>
		struct UserDataStruct<std::shared_ptr<T>>
		{
			static std::shared_ptr<T> Read(lua_State* lua, int index)
			{
				return SharedPtrProxy<T>::Read(lua, index);
			}

			static void Write(lua_State* lua, std::shared_ptr<T> data)
			{
				size_t size = sizeof(SharedPtrProxy<T>);
				new(lua_newuserdata(lua, size))SharedPtrProxy<T>(data);
				const std::string & typeName = lua::ClassFactory<T>::name;
				if (!typeName.empty())
				{
					lua_getglobal(lua, typeName.c_str());
					if (lua_istable(lua, -1))
					{
						lua_setmetatable(lua, -2);
					}
				}
			}
		};
	}

	namespace UserDataParameter
	{
		template<typename T>
		inline T Read(lua_State* lua, int index)
		{
			return UserDataStruct<T>::Read(lua, index);
		}

		template<typename T>
		inline void Write(lua_State* lua, T data)
		{
			UserDataStruct<T>::Write(lua, data);
		}
	}
}