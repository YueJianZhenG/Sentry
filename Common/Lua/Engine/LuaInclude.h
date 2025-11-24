#pragma once

#include <cassert>
#include <iostream>
#include <memory>
#include <cstring>
#include "Define.h"

#include "ParameterType.h"

namespace Lua
{
	inline bool is_lua_array(lua_State* L, int index, bool emy_as_arr = false) {
		if (lua_type(L, index) != LUA_TTABLE) return false;
		size_t raw_len = lua_rawlen(L, index);
		if (raw_len == 0 && !emy_as_arr) return false;
		index = lua_absindex(L, index);
		lua_pushnil(L);
		size_t curlen = 0;
		while (lua_next(L, index) != 0) {
			if (!lua_isinteger(L, -2)) return false;
			size_t key = lua_tointeger(L, -2);
			if (key <= 0 || key > raw_len) return false;
			lua_pop(L, 1);
			curlen++;
		}
		return curlen == raw_len;
	}

	template<typename T>
	inline std::enable_if_t<std::is_integral<T>::value, void> push_value(lua_State * L, T value)
	{
		lua_pushinteger(L, value);
	}

	template<typename T>
	inline std::enable_if_t<std::is_integral<T>::value, void> push_value(lua_State * L, const char * field, T value)
	{
		lua_pushstring(L, field);
		lua_pushinteger(L, value);
		lua_rawset(L, -3);
	}

	template<typename T>
	inline std::enable_if_t<std::is_enum<T>::value, void> push_value(lua_State * L, const char * field, T value)
	{
		lua_pushstring(L, field);
		lua_pushinteger(L, (int)value);
		lua_rawset(L, -3);
	}

	template<typename T>
	inline std::enable_if_t<std::is_floating_point<T>::value, void> push_value(lua_State * L, T value)
	{
		lua_pushnumber(L, value);
	}

	template<typename T>
	inline std::enable_if_t<std::is_floating_point<T>::value, void> push_value(lua_State * L, const char * field, T value)
	{
		lua_pushstring(L, field);
		lua_pushnumber(L, value);
		lua_rawset(L, -3);
	}

	inline void push_value(lua_State * L, const std::string & value)
	{
		lua_pushlstring(L, value.c_str(), value.size());
	}

	inline void push_value(lua_State * L, const char * field, const std::string & value)
	{
		lua_pushstring(L, field);
		lua_pushlstring(L, value.c_str(), value.size());
		lua_rawset(L, -3);
	}

	inline void push_value(lua_State * L, const char * value, size_t size)
	{
		lua_pushlstring(L, value, size);
	}

	inline void push_value(lua_State * L, const char * field, const char * value, size_t size)
	{
		lua_pushstring(L, field);
		lua_pushlstring(L, value, size);
		lua_rawset(L, -3);
	}

	inline std::string FormatFileAndLine(const char* file, const int line)
	{
		int length = (int)strlen(file);
		const char* fileName = nullptr;
		for (int index = length - 1; index >= 0; index--)
		{
#ifdef _WIN32
			if (file[index] == '\\')
#else
			if (file[index] == '/')
#endif
			{
				fileName = file + index + 1;
				break;
			}
		}
		fileName = fileName == nullptr ? file : fileName;
		char buffer[100] = {};
#ifdef _MSC_VER
		size_t size = sprintf_s(buffer, "%s:%d", fileName, line);
#else
		size_t size = sprintf(buffer, "%s:%d", fileName, line);
#endif
		return std::string(buffer, size);
	}

#define LuaDebugLog(msg)                                                                                            \
    {                                                                                                               \
        std::cout << "[Lua Error] : " << FromatFileAndLine(__FILE__, __LINE__) << "  [" << msg << "]" << std::endl; \
    }
#define LuaAssertLog(code, msg)                   \
    {                                             \
        if (code != 0) LuaDebugLog(#code << msg); \
    }
	inline bool lua_getfunction(lua_State* lua, int idx, const char* func)
	{
		if (lua_istable(lua, idx))
		{
			lua_getfield(lua, idx, func);
			if (lua_isfunction(lua, -1))
			{
				lua_remove(lua, -2);
				return true;
			}
			lua_remove(lua, -2);
		}
		return false;
	}
	inline bool lua_getfunction(lua_State* lua, const char* tab, const char* func)
	{
		lua_getglobal(lua, tab);
		return lua_getfunction(lua, -1, func);
	}

	inline int lua_reffunction(lua_State* lua, const char* tab, const char* func)
	{
		lua_getglobal(lua, tab);
		if (lua_istable(lua, -1))
		{
			lua_getfield(lua, -1, func);
			if (lua_isfunction(lua, -1))
			{
				lua_remove(lua, -2);
				return luaL_ref(lua, LUA_REGISTRYINDEX);
			}
			lua_remove(lua, -2);
		}
		return 0;
	}

#define lua_ref(lua) luaL_ref(lua, LUA_REGISTRYINDEX)
#define lua_unref(lua, ref) luaL_unref(lua, LUA_REGISTRYINDEX, ref)
#define lua_getref(lua, ref) lua_rawgeti(lua, LUA_REGISTRYINDEX, ref)

	inline void lua_pushtablefunction(lua_State* lua, const char* name, lua_CFunction func)
	{
		lua_pushcfunction(lua, func);
		lua_setfield(lua, -2, name);
	}

	inline void lua_pushglobalfunction(lua_State* lua, const char* name, lua_CFunction func)
	{
		lua_pushcfunction(lua, func);
		lua_setglobal(lua, name);
	}

}

namespace Lua
{
	namespace Parameter
	{
		template<typename T>
		inline std::enable_if_t<CommonParameter::IsCommonParameter<T>::value, T>
		Read(lua_State* lua, int index);

		template<typename T>
		inline std::enable_if_t<CommonParameter::IsCommonParameter<T>::value, void>
		Write(lua_State* lua, T data);

		template<typename T>
		inline std::enable_if_t<ContainerParameter::IsContainerParameter<T>::value, T>
		Read(lua_State* lua, int index);

		template<typename T>
		inline std::enable_if_t<ContainerParameter::IsContainerParameter<T>::value, void>
		Write(lua_State* lua, T data);

		template<typename T>
		inline std::enable_if_t<FunctionParameter::IsFunctionParameter<T>::value, T>
		Read(lua_State* lua, int index);

		template<typename T>
		inline std::enable_if_t<FunctionParameter::IsFunctionParameter<T>::value, void>
		Write(lua_State* lua, T data);

		template<typename T>
		inline std::enable_if_t<UserDataParameter::IsUserDataParameter<T>::value, T>
		Read(lua_State* lua, int index);

		template<typename T>
		inline std::enable_if_t<UserDataParameter::IsUserDataParameter<T>::value, void>
		Write(lua_State* lua, T data);

		template<typename T, typename... Args>
		inline void Encode(lua_State* lua, const T& t, Args... args);

		template<typename... Args>
		inline void WriteArgs(lua_State* lua, Args... args);
	}// namespace FromData


	template<typename T>
	struct PtrProxy
	{
	 public:
		explicit PtrProxy(T* t, bool del = false)
			: mNativePtr(t), mDelete(del) { }

		~PtrProxy()
		{
			if(this->mDelete) {
				delete this->mNativePtr;
			}
		}

	 public:
		static T* Read(lua_State* lua, int index)
		{
			if (lua_isuserdata(lua, index))
			{
				PtrProxy<T>* p = (PtrProxy<T>*)(lua_touserdata(lua, index));
				return p->mNativePtr;
			}
			return nullptr;
		}

		static void Write(lua_State* lua, T* data)
		{
			if (data == nullptr) return;
			new(lua_newuserdata(lua, sizeof(PtrProxy<T>))) PtrProxy<T>(data, false);
		}



	 private:
		bool mDelete;
		T* mNativePtr;
	};

	template<typename T>
	struct SharedPtrProxy
	{
	 public:
		explicit SharedPtrProxy(std::shared_ptr<T> t)
			: mNativePtr(t) { }

		~SharedPtrProxy() { printf("SharedPtrProxy\n");};

	 public:
		static std::shared_ptr<T> Read(lua_State* lua, int index)
		{
			if (lua_isuserdata(lua, index))
			{
				SharedPtrProxy<T>* p = (SharedPtrProxy<T>*)(lua_touserdata(lua, index));
				return p->mNativePtr;
			}
			return nullptr;
		}

		static void Write(lua_State* lua, std::shared_ptr<T> data)
		{
			if (data == nullptr) return;
			new(lua_newuserdata(lua, sizeof(SharedPtrProxy<T>))) SharedPtrProxy<T>(data, false);
		}

		static void Write(lua_State* lua, T* data)
		{
			if (data == nullptr) return;
			new(lua_newuserdata(lua, sizeof(PtrProxy<T>))) PtrProxy<T>(data, false);
		}

		static void Destroy(lua_State* lua, int index)
		{
			if (lua_isuserdata(lua, index))
			{
				SharedPtrProxy<T>* p = (SharedPtrProxy<T>*)(lua_touserdata(lua, index));
				if (p != nullptr)
				{
					p->~SharedPtrProxy();
				}
			}
		}

	 private:
		std::shared_ptr<T> mNativePtr;
	};


	template<typename T>
	struct UniquePtrProxy
	{
	public:
		explicit UniquePtrProxy(std::unique_ptr<T> t)
				: mNativePtr(std::move(t)) { }

		~UniquePtrProxy() = default;

	public:
		static std::unique_ptr<T> Read(lua_State* lua, int index)
		{
			if (lua_isuserdata(lua, index))
			{
				UniquePtrProxy<T>* p = (UniquePtrProxy<T>*)(lua_touserdata(lua, index));
				return std::move(p->mNativePtr);
			}
			return nullptr;
		}

		static void Write(lua_State* lua, std::unique_ptr<T> data)
		{
			if (data == nullptr) return;
			new(lua_newuserdata(lua, sizeof(UniquePtrProxy<T>))) UniquePtrProxy<T>(std::move(data));
		}
	private:
		std::unique_ptr<T> mNativePtr;
	};
}
