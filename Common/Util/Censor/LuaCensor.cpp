//
// Created by 64658 on 2025/6/16.
//

#include "LuaCensor.h"
#include "CensorFactory.h"
#include "Lua/Engine/UserDataParameter.h"

int censor::New(lua_State* L)
{
	censor::Factory * factory = new Factory();
	if(lua_isstring(L, 1))
	{
		size_t size = 0;
		const char* str = luaL_checklstring(L, 1, &size);
		if (!factory->LoadFromFile({ str, size }))
		{
			delete factory;
			luaL_error(L, "open file : %s", str);
			return 0;
		}
	}
	Lua::UserDataParameter::UserDataStruct<censor::Factory*>::WritePtr(L, factory);
	return 1;
}

int censor::Load(lua_State* L)
{
	censor::Factory* censorFactory = Lua::UserDataParameter::Read<censor::Factory*>(L, 1);
	if (censorFactory == nullptr)
	{
		luaL_error(L, "type CensorFactory");
		return 0;
	}
	size_t size = 0;
	const char* str = luaL_checklstring(L, 2, &size);
	if (!censorFactory->LoadFromFile({ str, size }))
	{
		luaL_error(L, "open file : %s", str);
		return 0;
	}
	censorFactory->Build();
	lua_pushboolean(L, true);
	return 1;
}

int censor::Insert(lua_State* L)
{
	censor::Factory* censorFactory = Lua::UserDataParameter::Read<censor::Factory*>(L, 1);
	if (censorFactory == nullptr)
	{
		luaL_error(L, "type CensorFactory");
		return 0;
	}
	size_t size = 0;
	const char* str = luaL_checklstring(L, 2, &size);
	censorFactory->Insert({ str, size});
	lua_pushboolean(L, true);
	return 1;
}

int censor::Mask(lua_State* L)
{
	censor::Factory* censorFactory = Lua::UserDataParameter::Read<censor::Factory*>(L, 1);
	if (censorFactory == nullptr)
	{
		luaL_error(L, "type CensorFactory");
		return 0;
	}
	char cc = '*';
	size_t size = 0;
	const char* text = luaL_checklstring(L, 2, &size);
	if(lua_type(L, 3) == LUA_TSTRING)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(L, 2, &count);
		if(str != nullptr && count > 0)
		{
			cc = str[0];
		}
	}
	std::string result(text, size);
	censorFactory->Mask(result, cc);
	lua_pushlstring(L, result.c_str(), result.size());
	return 1;
}

int censor::Check(lua_State* L)
{
	censor::Factory* censorFactory = Lua::UserDataParameter::Read<censor::Factory*>(L, 1);
	if (censorFactory == nullptr)
	{
		luaL_error(L, "type CensorFactory");
		return 0;
	}
	char cc = '*';
	size_t size = 0;
	const char* text = luaL_checklstring(L, 2, &size);
	if(lua_type(L, 3) == LUA_TSTRING)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(L, 2, &count);
		if(str != nullptr && count > 0)
		{
			cc = str[0];
		}
	}
	std::string result(text, size);
	lua_pushboolean(L, censorFactory->Check(result));
	return 1;
}

int censor::Build(lua_State* L)
{
	censor::Factory* censorFactory = Lua::UserDataParameter::Read<censor::Factory*>(L, 1);
	if (censorFactory == nullptr)
	{
		luaL_error(L, "type CensorFactory");
		return 0;
	}
	censorFactory->Build();
	return 0;
}