//
// Created by yy on 2025/11/8.
//

#pragma once
#include "LuaParameter.h"
#include "ClassNameProxy.h"

namespace lua
{
	template<typename T, typename V>
	class MemberProxy : public MemberBase
	{
	public:
		typedef V T::*Member;
		explicit MemberProxy(Member & member) : field(member) { }
	public:
		int Get(lua_State * luaEnv) final
		{
			T* ptr = Lua::PtrProxy<T>::Read(luaEnv, 1);
			Lua::Parameter::Write(luaEnv, ptr->*(field));
			return 1;
		}

		int Set(lua_State * luaEnv) final
		{
			T* ptr = Lua::PtrProxy<T>::Read(luaEnv, 1);
			ptr->*(field) = Lua::Parameter::Read<V>(luaEnv, 2);
			return 1;
		}
	private:
		Member field;
	};
}