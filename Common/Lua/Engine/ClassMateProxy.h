
#pragma once
#include "LuaInclude.h"
namespace Lua
{
	namespace ClassMateProxy
	{
		inline int Index(lua_State * lua)
		{
			const char* field = lua_tostring(lua, 2);
			if (lua_istable(lua, -1))
			{
				lua_getfield(lua, -1, field);
				if (!lua_isnil(lua, -1))
				{
					return 1;
				}
			}
			return 0;
		}

		template<typename T>
		inline int NewIndex(lua_State* lua)
		{
			size_t count = 0;
			const char* field = luaL_checklstring(lua, 2, &count);
			if (lua_getmetatable(lua, 1) != 0 && lua_istable(lua, -1))
			{
				static std::string name;
				name.assign(field, count);
				auto iter = lua::ClassFactory<T>::members.find(name);
				if(iter != lua::ClassFactory<T>::members.end())
				{
					return iter->second->Set(lua);
				}
			}
			return 0;
		}

		template<typename T>
		inline int GetIndex(lua_State* lua)
		{
			size_t count = 0;
			const char* field = luaL_checklstring(lua, 2, &count);
			if (lua_getmetatable(lua, 1) != 0 && lua_istable(lua, -1))
			{
				lua_getfield(lua, -1, field);
				if (!lua_isnil(lua, -1))
				{
					return 1;
				}
				static std::string name;
				name.assign(field, count);
				auto iter = lua::ClassFactory<T>::members.find(name);
				if(iter != lua::ClassFactory<T>::members.end())
				{
					return iter->second->Get(lua);
				}
			}

			if (lua_isnil(lua, -1))
			{
				lua_pop(lua, 1);
				for(const std::string & parent : lua::ClassFactory<T>::parents)
				{
					if (lua_getglobal(lua, parent.c_str()) && lua_istable(lua, -1))
					{
						lua_getfield(lua, -1, field);
						if (!lua_isnil(lua, -1))
						{
							return 1;
						}
						lua_pop(lua, 1);
					}
				}
			}
			return 0;
		}

		template<typename T>
		inline int OnDestroy(lua_State* lua)
		{
			PtrProxy<T>* p = nullptr;
			if (lua_isuserdata(lua, -1))
			{
				p = (PtrProxy<T>*)(lua_touserdata(lua, -1));
			}
			if(p != nullptr)
			{
				p->~PtrProxy();
			}
			return 0;
		}
	}// namespace ClassMateProxy
}