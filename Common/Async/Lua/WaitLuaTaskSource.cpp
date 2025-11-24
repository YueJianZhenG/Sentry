//
// Created by mac on 2022/3/31.
//

#include"WaitLuaTaskSource.h"
namespace acs
{
	WaitLuaTaskSource::WaitLuaTaskSource()
	{
		this->luaRef = 0;
		this->valRef = 0;
		this->mLua = nullptr;
	}

	WaitLuaTaskSource::~WaitLuaTaskSource()
	{
		if(this->luaRef > 0)
		{
			lua_unref(this->mLua, this->luaRef);
		}
		if(this->valRef > 0)
		{
			lua_unref(this->mLua, this->valRef);
		}
	}

	int WaitLuaTaskSource::SetResult(lua_State* lua)
	{
		WaitLuaTaskSource* luaTaskSource = Lua::PtrProxy<WaitLuaTaskSource>::Read(lua, 1);

		if(luaTaskSource != nullptr && luaTaskSource->ResumeTask())
		{
			luaTaskSource->mLua = lua;
			luaTaskSource->valRef = luaL_ref(lua, LUA_REGISTRYINDEX);
			lua_pushthread(lua);
			luaTaskSource->luaRef = luaL_ref(lua, LUA_REGISTRYINDEX); //保留lua的引用
		}
		return 0;
	}
}

