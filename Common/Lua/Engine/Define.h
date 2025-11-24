
#pragma once

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
};


class ILuaWrite
{
public:
	virtual  ~ILuaWrite() = default;
	virtual int WriteToLua(lua_State* lua) const = 0;
};